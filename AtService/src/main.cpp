#include <csignal>
#include <cstdlib>

import std;
import sample.core.log;
import sample.error;
import at_service.core.app;
import at_service.core.app.config;

int main(int argc, char* argv[])
{
    std::signal(SIGPIPE, SIG_IGN);
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGUSR1);
    if(const int err = pthread_sigmask(SIG_BLOCK, &mask, nullptr); err != 0) {
        sample::Log::force_error("Failed to block signals, error code `{0}`", err);
        return EXIT_FAILURE;
    }
    try {
        std::expected<
            std::pair<std::string, at_service::AppConfig>,
            at_service::AppConfig::ErrorStatus>
            app_config_exp = at_service::AppConfig::from_console(argc, argv);
        if(!app_config_exp) {
            sample::Log::print(app_config_exp.error().message);
            return app_config_exp.error().code;
        }
        const std::string config_path{std::move(app_config_exp->first)};
        at_service::AppConfig config{std::move(app_config_exp->second)};
        at_service::App app(config);
        app.start();
        while(true) {
            int caught_signal{0};
            if(const int result = sigwait(&mask, &caught_signal); result == 0) {
                sample::Log::force_info("Received OS signal `{0}`", caught_signal);
                switch(caught_signal) {
                case SIGHUP: {
                    std::expected<at_service::AppConfig, at_service::AppConfig::ErrorStatus>
                        config_exp = at_service::AppConfig::from_config_file(config_path);
                    if(!config_exp) {
                        sample::Log::print(config_exp.error().message);
                        continue;
                    }
                    if(at_service::AppConfig config_new{*std::move(config_exp)};
                       app.reconfigure(config_new))
                    {
                        config = std::move(config_new);
                    }
                    continue;
                }
                case SIGUSR1:
                    app.update_dictionary(config.dictionary_path);
                    continue;
                case SIGTERM:
                case SIGQUIT:
                case SIGINT:
                    return EXIT_SUCCESS;
                default:
                    sample::Log::force_error("Received OS unexpected signal `{0}`", caught_signal);
                    return EXIT_FAILURE;
                }
            } else {
                sample::Log::force_error("sigwait failed, error code `{0}`", result);
                return EXIT_FAILURE;
            }
        }
    }
    catch(const sample::error::Exception& e) {
        const std::source_location& loc = e.where();
        sample::Log::print(
            "Fatal error\n"
            "===================================\n"
            "Exception: {0}\n"
            "Message:   {1}\n"
            "Location:  {2}:{3}\n"
            "Function:  {4}\n"
            "===================================",
            typeid(e).name(),
            e.what(),
            loc.file_name(),
            loc.line(),
            loc.function_name());
        return EXIT_FAILURE;
    }
    catch(const std::exception& e) {
        sample::Log::print(
            "Fatal error\n"
            "===================================\n"
            "Exception: {0}\n"
            "Message:   {1}\n"
            "===================================",
            typeid(e).name(),
            e.what());
        return EXIT_FAILURE;
    }
    catch(...) {
        sample::Log::print("Fatal unknown error");
        return EXIT_FAILURE;
    }
}