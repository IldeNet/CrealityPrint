#ifndef OCTOPRINT_MANAGER_HPP
#define OCTOPRINT_MANAGER_HPP

#include <string>
#include <functional>
#include <vector>

#include <wx/string.h>

namespace Slic3r {
namespace GUI {

class OctoPrintManager {
public:
    struct OctoPrintConfig {
        std::string host;
        int port;
        std::string api_key;
        std::string filename;
        bool start_print;
        std::string folder;
    };

    struct PrinterState {
        std::string state;
        double progress;
        int print_time;
        int print_time_left;
        std::string filename;
    };

    using ProgressCallback = std::function<void(int percent)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    using SuccessCallback = std::function<void()>;

    OctoPrintManager();
    ~OctoPrintManager();

    bool test_connection(const std::string& host, int port, const std::string& api_key);
    bool upload_file(const std::string& filepath, const OctoPrintConfig& config, 
                     ProgressCallback progress_cb = nullptr,
                     ErrorCallback error_cb = nullptr,
                     SuccessCallback success_cb = nullptr);
    
    bool get_printer_state(PrinterState& state);
    bool start_print(const std::string& filename);
    bool cancel_print();
    
    std::vector<std::string> get_files();
    bool delete_file(const std::string& filename);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace GUI
} // namespace Slic3r

#endif // OCTOPRINT_MANAGER_HPP
