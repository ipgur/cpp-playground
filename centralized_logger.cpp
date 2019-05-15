#include <string>
#include <fstream>
#include <iostream>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <thread>

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

enum severity_level
{
    normal,
    warning,
    error
};

// Complete sink type
using file_sink = boost::log::sinks::asynchronous_sink< boost::log::sinks::text_file_backend >;

struct LoggerProperties {
    LoggerProperties() = default;
    LoggerProperties &operator=(const LoggerProperties &) = default;
    LoggerProperties(const LoggerProperties &) = default;
    std::string m_logFileNamePrefix = "/var/tmp/sample";
    std::string m_collectorDir = "/var/tmp/logs";
    severity_level m_severityLevel = warning;
    uint64_t m_rotateSizeBytes = 50 * 1024 * 1024;
};

class CentralizedLogger {
public:
     boost::shared_ptr< file_sink > init_logging() {
         boost::shared_ptr< logging::core > core = logging::core::get();

         boost::shared_ptr< sinks::text_file_backend > backendFile =
                 boost::make_shared< sinks::text_file_backend >(
                         keywords::file_name = m_loggerProperties.m_logFileNamePrefix + "_%Y%m%d.log",
                         // rotate the file upon reaching 50 MB size
                         keywords::rotation_size = m_loggerProperties.m_rotateSizeBytes
                 );

         backendFile->auto_flush(true);

         boost::shared_ptr<file_sink> fileSink(new file_sink(backendFile));

         // manage filtering through the sink interface
         fileSink->set_filter(expr::attr< severity_level >("Severity") >= m_loggerProperties.m_severityLevel);

         fileSink->locked_backend()->set_file_collector(sinks::file::make_collector(
                 keywords::target = m_loggerProperties.m_collectorDir
         ));

         logging::core::get()->add_sink(fileSink);

         return fileSink;
    }
    //]

    //[ example_sinks_async_stop
    void stop_logging(boost::shared_ptr< file_sink >& sink) {
        boost::shared_ptr< logging::core > core = logging::core::get();

        // Remove the sink from the core, so that no records are passed to it
        core->remove_sink(sink);

        // Break the feeding loop
        sink->stop();

        // Flush all log records that may have left buffered
        sink->flush();

        sink.reset();
    }

private:
    LoggerProperties m_loggerProperties;
};

void print_msg() {
    src::severity_channel_logger< severity_level > lg(keywords::channel = "net");
    BOOST_LOG_SEV(lg, warning) << "Hello world!This is earth!";
}

int main(int, char*[]) {
    LoggerProperties loggerProperties;
    CentralizedLogger centralizedLogger(std::move(loggerProperties));
    boost::shared_ptr< file_sink > sink = centralizedLogger.init_logging();
    src::severity_channel_logger< severity_level > lg(keywords::channel = "net");
    BOOST_LOG_SEV(lg, warning) << "Hello world!This is world!";

    std::thread t[5];

    for(int i = 0; i < 5; ++i) {
        t[i] = std::move(std::thread(&print_msg));
    }

    for(int i = 0; i < 5; ++i) {
        t[i].join();
    }

    centralizedLogger.stop_logging(sink);

    return 0;
}
