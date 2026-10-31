#include "Logger.hpp"
#include "RollingLogFollow.hpp"

#include "../../event/EventBus.hpp"

#include "../../config/ConfigValue.hpp"

#include <filesystem>
#include <fstream>
#include <sys/stat.h>

using namespace Log;

CLogger::CLogger() : m_isTrace(Env::isTrace()) {
    m_logger.setLogLevel(m_isTrace ? Hyprutils::CLI::LOG_TRACE : Hyprutils::CLI::LOG_DEBUG);
}

void CLogger::log(Hyprutils::CLI::eLogLevel level, const std::string_view& str) {
    if (!m_logsEnabled)
        return;

    if (level == Hyprutils::CLI::LOG_TRACE && !m_isTrace)
        return;

    if (SRollingLogFollow::get().isRunning())
        SRollingLogFollow::get().addLog(str);

    m_logger.log(level, str);
}

void CLogger::log(Hyprutils::CLI::eLogLevel level, const std::string_view loc, const std::string_view str) {
    if (!m_logsEnabled)
        return;

    if (level == Hyprutils::CLI::LOG_TRACE && !m_isTrace)
        return;

    log(level, std::format("[{}] {}", loc, str));
}

void CLogger::initIS(const std::string_view& IS) {
    m_logFile = std::string{IS} + (ISDEBUG ? "/hyprlandd.log" : "/hyprland.log");

    std::ofstream bootstrap(m_logFile, std::ios::out | std::ios::noreplace);
    struct stat   identity = {};
    if (bootstrap && fstat(bootstrap.native_handle(), &identity) == 0)
        m_bootstrapLog = SFileIdentity{.device = identity.st_dev, .inode = identity.st_ino};

    if (m_logger.setOutputFile(m_logFile))
        m_activeLogFile = m_logFile;
    m_logger.setEnableRolling(true);
    m_logger.setEnableColor(false);
    m_logger.setEnableStdout(true);
    m_logger.setTime(false);
}

void CLogger::initCallbacks() {
    static auto P = Event::bus()->m_events.config.reloaded.listen([this]() { recheckCfg(); });
    recheckCfg();
}

void CLogger::recheckCfg() {
    static auto PDISABLELOGS  = CConfigValue<Config::INTEGER>("debug:disable_logs");
    static auto PDISABLETIME  = CConfigValue<Config::INTEGER>("debug:disable_time");
    static auto PENABLESTDOUT = CConfigValue<Config::INTEGER>("debug:enable_stdout_logs");
    static auto PENABLEFILE   = CConfigValue<Config::INTEGER>("debug:enable_file_logs");
    static auto PENABLECOLOR  = CConfigValue<Config::INTEGER>("debug:colored_stdout_logs");

    configureLogs(!*PDISABLELOGS, *PENABLEFILE, *PENABLESTDOUT);
    m_logger.setTime(!*PDISABLETIME);
    m_logger.setEnableColor(*PENABLECOLOR);
}

void CLogger::configureLogs(bool logsEnabled, bool fileLogsEnabled, bool stdoutEnabled) {
    m_logsEnabled = logsEnabled;

    const std::string outputFile = fileLogsEnabled ? m_logFile : (stdoutEnabled ? "" : "/dev/null");
    if (outputFile != m_activeLogFile && m_logger.setOutputFile(outputFile))
        m_activeLogFile = outputFile;

    m_logger.setEnableStdout(logsEnabled && stdoutEnabled);

    if (!std::exchange(m_firstConfig, false) || fileLogsEnabled || !m_bootstrapLog)
        return;

    struct stat identity = {};
    if (lstat(m_logFile.c_str(), &identity) != 0 || !S_ISREG(identity.st_mode) || identity.st_dev != m_bootstrapLog->device || identity.st_ino != m_bootstrapLog->inode)
        return;

    std::error_code ec;
    std::filesystem::remove(m_logFile, ec);
    if (ec)
        log(WARN, std::format("Failed to remove bootstrap log {}: {}", m_logFile, ec.message()));
}

const std::string& CLogger::rolling() {
    return m_logger.rollingLog();
}

Hyprutils::CLI::CLogger& CLogger::hu() {
    return m_logger;
}
