#include <debug/log/Logger.hpp>
#include <macros.hpp>
#include <debug/log/RollingLogFollow.hpp>

#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <random>

namespace Log {
    class CLoggerTestAccessor {
      public:
        static void configure(CLogger& logger, bool logsEnabled, bool fileLogsEnabled, bool stdoutEnabled) {
            logger.configureLogs(logsEnabled, fileLogsEnabled, stdoutEnabled);
        }
    };
}

class CLoggerTest : public testing::Test {
  protected:
    void SetUp() override {
        m_directory = std::filesystem::temp_directory_path() / std::format("hyprland-logger-test-{}", std::random_device{}());
        ASSERT_TRUE(std::filesystem::create_directory(m_directory));
        m_logger = makeUnique<Log::CLogger>();
        m_file   = m_directory / (ISDEBUG ? "hyprlandd.log" : "hyprland.log");
    }

    void TearDown() override {
        m_logger.reset();
        std::error_code ec;
        std::filesystem::remove_all(m_directory, ec);
    }

    void init() {
        m_logger->initIS(m_directory.string());
    }

    void configure(bool logsEnabled = true, bool fileLogsEnabled = true, bool stdoutEnabled = false) {
        Log::CLoggerTestAccessor::configure(*m_logger, logsEnabled, fileLogsEnabled, stdoutEnabled);
    }

    std::string contents() {
        std::ifstream file(m_file);
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    void closeFile() {
        ASSERT_TRUE(m_logger->hu().setOutputFile(""));
    }

    std::filesystem::path m_directory;
    std::filesystem::path m_file;
    UP<Log::CLogger>      m_logger;
};

TEST_F(CLoggerTest, defaultFileLoggingIncludesBootstrap) {
    init();
    m_logger->log(Log::INFO, "bootstrap message");
    configure();
    m_logger->log(Log::INFO, "configured message");
    closeFile();

    EXPECT_NE(contents().find("bootstrap message"), std::string::npos);
    EXPECT_NE(contents().find("configured message"), std::string::npos);
}

TEST_F(CLoggerTest, unrelatedReloadPreservesFileHistory) {
    init();
    configure();
    m_logger->log(Log::INFO, "before reload");
    configure();
    configure();
    m_logger->log(Log::INFO, "after reload");
    closeFile();

    EXPECT_NE(contents().find("before reload"), std::string::npos);
    EXPECT_NE(contents().find("after reload"), std::string::npos);
}

TEST_F(CLoggerTest, disabledFileKeepsStdoutRollingAndFollowLogs) {
    init();
    m_logger->log(Log::INFO, "bootstrap message");
    configure(true, false, true);
    ASSERT_FALSE(std::filesystem::exists(m_file));

    auto& follow = Log::SRollingLogFollow::get();
    follow.startFor(-1);
    testing::internal::CaptureStdout();
    m_logger->log(Log::INFO, "journal message");
    const auto stdoutLog = testing::internal::GetCapturedStdout();
    const auto followLog = follow.takeLog(-1);
    follow.stopFor(-1);

    EXPECT_NE(stdoutLog.find("journal message"), std::string::npos);
    EXPECT_NE(m_logger->rolling().find("bootstrap message"), std::string::npos);
    EXPECT_NE(m_logger->rolling().find("journal message"), std::string::npos);
    EXPECT_NE(followLog.find("journal message"), std::string::npos);
    EXPECT_FALSE(std::filesystem::exists(m_file));
}

TEST_F(CLoggerTest, disabledFileAndStdoutKeepRollingLogsAcrossReloads) {
    init();
    configure(true, false, false);
    m_logger->log(Log::INFO, "rolling message");
    configure(true, false, false);
    m_logger->log(Log::INFO, "after reload");

    EXPECT_NE(m_logger->rolling().find("rolling message"), std::string::npos);
    EXPECT_NE(m_logger->rolling().find("after reload"), std::string::npos);
    EXPECT_FALSE(std::filesystem::exists(m_file));
}

TEST_F(CLoggerTest, dependencyConnectionKeepsRollingWhenSinksAreDisabled) {
    init();
    configure(true, false, false);
    Hyprutils::CLI::CLoggerConnection connection(m_logger->hu());
    connection.setName("dependency");
    connection.log(Log::INFO, "dependency message");

    EXPECT_NE(m_logger->rolling().find("dependency message"), std::string::npos);
    EXPECT_FALSE(std::filesystem::exists(m_file));
}

TEST_F(CLoggerTest, masterDisableSuppressesStdoutRollingAndFollowLogs) {
    init();
    configure(true, false, true);
    m_logger->log(Log::INFO, "before disable");
    configure(false, false, true);
    const auto before = m_logger->rolling();
    auto&      follow = Log::SRollingLogFollow::get();
    follow.startFor(-1);
    follow.takeLog(-1);
    testing::internal::CaptureStdout();
    m_logger->log(Log::INFO, "disabled message");
    m_logger->log(Log::INFO, "test", "disabled location message");
    m_logger->log(Log::INFO, "test", "disabled formatted message {}", 42);
    const auto stdoutLog = testing::internal::GetCapturedStdout();
    const auto followLog = follow.takeLog(-1);
    follow.stopFor(-1);

    EXPECT_TRUE(stdoutLog.empty());
    EXPECT_TRUE(followLog.empty());
    EXPECT_EQ(m_logger->rolling(), before);
    EXPECT_FALSE(std::filesystem::exists(m_file));
}

TEST_F(CLoggerTest, disablingAfterFirstConfigRetainsFileHistory) {
    init();
    configure();
    m_logger->log(Log::INFO, "retained message");
    configure(true, false, true);
    m_logger->log(Log::INFO, "journal only message");

    EXPECT_TRUE(std::filesystem::exists(m_file));
    EXPECT_NE(contents().find("retained message"), std::string::npos);
    EXPECT_EQ(contents().find("journal only message"), std::string::npos);
}

TEST_F(CLoggerTest, explicitReenableStartsANewFile) {
    init();
    configure();
    m_logger->log(Log::INFO, "previous file message");
    configure(true, false, true);
    configure(true, true, true);
    m_logger->log(Log::INFO, "new file message");
    closeFile();

    EXPECT_EQ(contents().find("previous file message"), std::string::npos);
    EXPECT_NE(contents().find("new file message"), std::string::npos);
}

TEST_F(CLoggerTest, firstConfigDoesNotRemovePreexistingFile) {
    std::ofstream(m_file) << "preexisting message";
    init();
    configure(true, false, true);

    EXPECT_TRUE(std::filesystem::exists(m_file));
}

TEST_F(CLoggerTest, firstConfigDoesNotRemoveReplacedBootstrapFile) {
    init();
    std::filesystem::rename(m_file, m_directory / "original.log");
    std::ofstream(m_file) << "replacement message";
    configure(true, false, true);

    EXPECT_EQ(contents(), "replacement message");
}

TEST_F(CLoggerTest, firstConfigDoesNotRemoveBootstrapSymlinkReplacement) {
    init();
    std::filesystem::rename(m_file, m_directory / "original.log");
    std::ofstream(m_directory / "target.log") << "target message";
    std::filesystem::create_symlink("target.log", m_file);
    configure(true, false, true);

    EXPECT_TRUE(std::filesystem::is_symlink(m_file));
    EXPECT_EQ(contents(), "target message");
}
