#ifndef VISASESSION_H
#define VISASESSION_H

#include <string>
#include <visa.h>

namespace AMAS {

class VISASession {
public:
    VISASession();
    ~VISASession();

    // Prevent copy/assignment to avoid double-free of VISA sessions
    VISASession(const VISASession&) = delete;
    VISASession& operator=(const VISASession&) = delete;

    // Allow move semantics
    VISASession(VISASession&& other) noexcept;
    VISASession& operator=(VISASession&& other) noexcept;

    // Connection lifecycle
    bool connect(const std::string& resourceStr, unsigned int timeoutMs = 5000);
    void disconnect();
    bool isConnected() const;

    // Basic I/O operations
    bool write(const std::string& command);
    bool read(std::string& outResponse, size_t bufferSize = 4096);
    bool query(const std::string& command, std::string& outResponse, size_t bufferSize = 4096);

    // Attribute management
    bool setTimeout(unsigned int timeoutMs);

    // Get raw handles if needed
    ViSession getSession() const { return m_instrSession; }
    ViSession getRM() const { return m_rmSession; }

private:
    ViSession m_rmSession;
    ViSession m_instrSession;
    bool m_isConnected;
    std::string m_currentResource;

    bool checkStatus(ViStatus status, const std::string& context) const;
};

} // namespace AMAS

#endif // VISASESSION_H
