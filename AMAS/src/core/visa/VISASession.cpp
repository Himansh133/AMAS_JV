#include "VISASession.h"
#include "Logger.h"
#include <cstring>
#include <vector>

namespace AMAS {

VISASession::VISASession()
    : m_rmSession(VI_NULL)
    , m_instrSession(VI_NULL)
    , m_isConnected(false) {
}

VISASession::~VISASession() {
    disconnect();
}

VISASession::VISASession(VISASession&& other) noexcept
    : m_rmSession(other.m_rmSession)
    , m_instrSession(other.m_instrSession)
    , m_isConnected(other.m_isConnected)
    , m_currentResource(std::move(other.m_currentResource)) {
    other.m_rmSession = VI_NULL;
    other.m_instrSession = VI_NULL;
    other.m_isConnected = false;
}

VISASession& VISASession::operator=(VISASession&& other) noexcept {
    if (this != &other) {
        disconnect();
        m_rmSession = other.m_rmSession;
        m_instrSession = other.m_instrSession;
        m_isConnected = other.m_isConnected;
        m_currentResource = std::move(other.m_currentResource);

        other.m_rmSession = VI_NULL;
        other.m_instrSession = VI_NULL;
        other.m_isConnected = false;
    }
    return *this;
}

bool VISASession::connect(const std::string& resourceStr, unsigned int timeoutMs) {
    disconnect();

    Log::info("Opening VISA Default Resource Manager...");
    ViStatus status = viOpenDefaultRM(&m_rmSession);
    if (status < VI_SUCCESS) {
        checkStatus(status, "viOpenDefaultRM");
        return false;
    }

    Log::info("Opening VISA session to instrument: " + resourceStr);
    status = viOpen(m_rmSession, reinterpret_cast<ViConstRsrc>(resourceStr.c_str()), 
                    VI_NULL, VI_NULL, &m_instrSession);
    
    if (status < VI_SUCCESS) {
        checkStatus(status, "viOpen (" + resourceStr + ")");
        viClose(m_rmSession);
        m_rmSession = VI_NULL;
        return false;
    }

    m_currentResource = resourceStr;
    m_isConnected = true;

    // Set default timeout
    setTimeout(timeoutMs);

    Log::info("VISA Session successfully established with " + resourceStr);
    return true;
}

void VISASession::disconnect() {
    if (m_instrSession != VI_NULL) {
        Log::info("Closing VISA instrument session for: " + m_currentResource);
        viClose(m_instrSession);
        m_instrSession = VI_NULL;
    }
    if (m_rmSession != VI_NULL) {
        Log::info("Closing VISA Resource Manager session.");
        viClose(m_rmSession);
        m_rmSession = VI_NULL;
    }
    m_isConnected = false;
    m_currentResource.clear();
}

bool VISASession::isConnected() const {
    return m_isConnected && (m_instrSession != VI_NULL);
}

bool VISASession::setTimeout(unsigned int timeoutMs) {
    if (!isConnected()) {
        Log::error("Cannot set timeout: VISA session not connected.");
        return false;
    }
    ViStatus status = viSetAttribute(m_instrSession, VI_ATTR_TMO_VALUE, timeoutMs);
    return checkStatus(status, "viSetAttribute (VI_ATTR_TMO_VALUE)");
}

bool VISASession::write(const std::string& command) {
    if (!isConnected()) {
        Log::error("Cannot write command: VISA session not connected. Command: " + command);
        return false;
    }

    // Ensure command ends with a newline character, required by SCPI instruments
    std::string formattedCmd = command;
    if (formattedCmd.empty() || formattedCmd.back() != '\n') {
        formattedCmd += "\n";
    }

    ViUInt32 bytesWritten = 0;
    ViStatus status = viWrite(m_instrSession, 
                              reinterpret_cast<ViConstBuf>(formattedCmd.c_str()), 
                              static_cast<ViUInt32>(formattedCmd.size()), 
                              &bytesWritten);

    if (status < VI_SUCCESS) {
        checkStatus(status, "viWrite (" + command + ")");
        return false;
    }
    return true;
}

bool VISASession::read(std::string& outResponse, size_t bufferSize) {
    if (!isConnected()) {
        Log::error("Cannot read: VISA session not connected.");
        return false;
    }

    std::vector<char> buffer(bufferSize, 0);
    ViUInt32 bytesRead = 0;
    
    ViStatus status = viRead(m_instrSession, 
                             reinterpret_cast<ViPBuf>(buffer.data()), 
                             static_cast<ViUInt32>(bufferSize - 1), 
                             &bytesRead);

    if (status < VI_SUCCESS) {
        checkStatus(status, "viRead");
        return false;
    }

    outResponse = std::string(buffer.data(), bytesRead);
    
    // Strip trailing carriage return or newline characters
    while (!outResponse.empty() && (outResponse.back() == '\n' || outResponse.back() == '\r')) {
        outResponse.pop_back();
    }

    return true;
}

bool VISASession::query(const std::string& command, std::string& outResponse, size_t bufferSize) {
    if (!write(command)) {
        return false;
    }
    return read(outResponse, bufferSize);
}

bool VISASession::checkStatus(ViStatus status, const std::string& context) const {
    if (status < VI_SUCCESS) {
        char errDesc[256] = {0};
        // Obtain a human-readable description of the error from VISA
        if (m_rmSession != VI_NULL) {
            viStatusDesc(m_rmSession, status, errDesc);
        } else {
            // Fallback if resource manager is not available
            std::strncpy(errDesc, "VISA Error (Resource Manager not initialized)", sizeof(errDesc) - 1);
        }
        
        Log::error("VISA Error in " + context + ". Status Code: 0x" + 
                   std::string(errDesc) + " (0x" + std::to_string(status) + ")");
        return false;
    }
    return true;
}

} // namespace AMAS
