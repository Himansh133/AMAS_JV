#include "reports/ReportGenerator.h"
#include "measurement/MeasurementProcessor.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <iomanip>

namespace AMAS {

// ── Markdown Report Implementation ──────────────────────────────────────────
MarkdownReport::MarkdownReport(const MeasurementSession& session) {
    buildReport(session);
}

std::string MarkdownReport::getContent() const {
    return m_content;
}

void MarkdownReport::buildReport(const MeasurementSession& session) {
    std::stringstream ss;
    ss << "# AMAS Measurement & Analysis Report\n\n";
    ss << "## Session Summary\n";
    ss << "| Parameter | Value |\n";
    ss << "| :--- | :--- |\n";
    ss << "| **Session Name** | " << session.sessionName << " |\n";
    ss << "| **Measurement Type** | " << session.measurementType << " |\n";
    ss << "| **Timestamp** | " << session.timestamp << " |\n";
    ss << "| **RF Band** | " << session.bandName << " |\n";
    ss << "| **Calibration File** | " << (session.calibrationFile.empty() ? "None" : session.calibrationFile) << " |\n";
    ss << "| **Output Folder** | `" << session.outputFolder << "` |\n\n";

    ss << "## Device Under Test (DUT) & Operator Info\n";
    ss << "| Parameter | Value |\n";
    ss << "| :--- | :--- |\n";
    ss << "| **Antenna Model** | " << session.metadata.antennaModel << " |\n";
    ss << "| **Operator Name** | " << session.metadata.operatorName << " |\n";
    ss << "| **Operator Notes** | " << (session.metadata.notes.empty() ? "N/A" : session.metadata.notes) << " |\n\n";

    // Perform Pattern Analysis if there are angular sweeps
    if (!session.results.empty() && session.results.back().angleDeg != session.results.front().angleDeg) {
        std::vector<float> angles;
        std::vector<double> mags;
        angles.reserve(session.results.size());
        mags.reserve(session.results.size());
        for (const auto& pt : session.results) {
            angles.push_back(pt.angleDeg);
            mags.push_back(pt.magnitudeDb);
        }

        auto metrics = MeasurementProcessor::analyzePattern(angles, mags);

        ss << "## Radiation Pattern Analysis Results\n";
        ss << "| Metric | Value |\n";
        ss << "| :--- | :--- |\n";
        ss << "| **Peak Gain** | " << std::fixed << std::setprecision(2) << metrics.peakGainDb << " dBi |\n";
        ss << "| **Beam Direction** | " << std::fixed << std::setprecision(1) << metrics.beamDirectionDeg << " deg |\n";
        ss << "| **3dB Beamwidth (HPBW)** | " << std::fixed << std::setprecision(1) << metrics.beamwidth3dB << " deg |\n";
        ss << "| **10dB Beamwidth** | " << std::fixed << std::setprecision(1) << metrics.beamwidth10dB << " deg |\n";
        ss << "| **Front-to-Back Ratio** | " << std::fixed << std::setprecision(2) << metrics.frontToBackRatioDb << " dB |\n";
        ss << "| **Side Lobe Level (SLL)** | " << std::fixed << std::setprecision(2) << metrics.sideLobeLevelDb << " dB |\n\n";
    }

    ss << "## Measured Data Points (Subset - First 10 Points)\n";
    ss << "| Angle (deg) | Frequency (Hz) | Magnitude (dB) | Phase (deg) | Return Loss (dB) | VSWR |\n";
    ss << "| :--- | :--- | :--- | :--- | :--- | :--- |\n";
    size_t count = std::min(session.results.size(), size_t(10));
    for (size_t i = 0; i < count; ++i) {
        const auto& pt = session.results[i];
        ss << "| " << std::fixed << std::setprecision(1) << pt.angleDeg
           << " | " << std::fixed << std::setprecision(0) << pt.frequencyHz
           << " | " << std::fixed << std::setprecision(2) << pt.magnitudeDb
           << " | " << std::fixed << std::setprecision(2) << pt.phaseDeg
           << " | " << std::fixed << std::setprecision(2) << pt.returnLossDb
           << " | " << std::fixed << std::setprecision(2) << pt.vswr << " |\n";
    }
    if (session.results.size() > 10) {
        ss << "| ... | ... | ... | ... | ... | ... |\n";
    }
    
    m_content = ss.str();
}

// ── HTML Report Implementation ──────────────────────────────────────────────
HtmlReport::HtmlReport(const MeasurementSession& session) {
    buildReport(session);
}

std::string HtmlReport::getContent() const {
    return m_content;
}

void HtmlReport::buildReport(const MeasurementSession& session) {
    std::stringstream ss;
    ss << "<!DOCTYPE html>\n<html>\n<head>\n";
    ss << "  <title>AMAS Report - " << session.sessionName << "</title>\n";
    ss << "  <style>\n";
    ss << "    body { font-family: 'Segoe UI', Arial, sans-serif; margin: 40px; background-color: #f7f9fa; color: #333; }\n";
    ss << "    .report-container { max-width: 900px; margin: 0 auto; background: white; padding: 40px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.08); }\n";
    ss << "    h1 { color: #1e3d59; border-bottom: 2px solid #ff6e40; padding-bottom: 10px; margin-bottom: 30px; }\n";
    ss << "    h2 { color: #17b978; margin-top: 30px; border-left: 4px solid #17b978; padding-left: 10px; }\n";
    ss << "    table { width: 100%; border-collapse: collapse; margin: 15px 0; }\n";
    ss << "    th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n";
    ss << "    th { background-color: #f2f2f2; color: #1e3d59; font-weight: bold; }\n";
    ss << "    .metric-value { font-weight: bold; color: #ff6e40; }\n";
    ss << "    .footer { margin-top: 50px; font-size: 0.9em; color: #777; border-top: 1px solid #eee; padding-top: 15px; text-align: center; }\n";
    ss << "  </style>\n</head>\n<body>\n";
    ss << "<div class='report-container'>\n";
    ss << "  <h1>AMAS Measurement & Analysis Report</h1>\n";

    ss << "  <h2>Session Summary</h2>\n";
    ss << "  <table>\n";
    ss << "    <tr><th>Parameter</th><th>Value</th></tr>\n";
    ss << "    <tr><td>Session Name</td><td>" << session.sessionName << "</td></tr>\n";
    ss << "    <tr><td>Measurement Type</td><td>" << session.measurementType << "</td></tr>\n";
    ss << "    <tr><td>Timestamp</td><td>" << session.timestamp << "</td></tr>\n";
    ss << "    <tr><td>RF Band</td><td>" << session.bandName << "</td></tr>\n";
    ss << "    <tr><td>Calibration File</td><td>" << (session.calibrationFile.empty() ? "None" : session.calibrationFile) << "</td></tr>\n";
    ss << "    <tr><td>Output Folder</td><td>" << session.outputFolder << "</td></tr>\n";
    ss << "  </table>\n";

    ss << "  <h2>Device Under Test (DUT) & Operator</h2>\n";
    ss << "  <table>\n";
    ss << "    <tr><td>Antenna Model</td><td>" << session.metadata.antennaModel << "</td></tr>\n";
    ss << "    <tr><td>Operator Name</td><td>" << session.metadata.operatorName << "</td></tr>\n";
    ss << "    <tr><td>Operator Notes</td><td>" << (session.metadata.notes.empty() ? "N/A" : session.metadata.notes) << "</td></tr>\n";
    ss << "  </table>\n";

    if (!session.results.empty() && session.results.back().angleDeg != session.results.front().angleDeg) {
        std::vector<float> angles;
        std::vector<double> mags;
        angles.reserve(session.results.size());
        mags.reserve(session.results.size());
        for (const auto& pt : session.results) {
            angles.push_back(pt.angleDeg);
            mags.push_back(pt.magnitudeDb);
        }
        auto metrics = MeasurementProcessor::analyzePattern(angles, mags);

        ss << "  <h2>Radiation Pattern Analysis</h2>\n";
        ss << "  <table>\n";
        ss << "    <tr><th>Metric</th><th>Calculated Value</th></tr>\n";
        ss << "    <tr><td>Peak Gain</td><td class='metric-value'>" << std::fixed << std::setprecision(2) << metrics.peakGainDb << " dBi</td></tr>\n";
        ss << "    <tr><td>Beam Direction</td><td>" << std::fixed << std::setprecision(1) << metrics.beamDirectionDeg << " deg</td></tr>\n";
        ss << "    <tr><td>3dB Beamwidth (HPBW)</td><td>" << std::fixed << std::setprecision(1) << metrics.beamwidth3dB << " deg</td></tr>\n";
        ss << "    <tr><td>10dB Beamwidth</td><td>" << std::fixed << std::setprecision(1) << metrics.beamwidth10dB << " deg</td></tr>\n";
        ss << "    <tr><td>Front-to-Back Ratio</td><td>" << std::fixed << std::setprecision(2) << metrics.frontToBackRatioDb << " dB</td></tr>\n";
        ss << "    <tr><td>Side Lobe Level (SLL)</td><td>" << std::fixed << std::setprecision(2) << metrics.sideLobeLevelDb << " dB</td></tr>\n";
        ss << "  </table>\n";
    }

    ss << "  <div class='footer'>\n";
    ss << "    Report generated by AMAS (Antenna Measurement & Analysis Software) Backend Core.\n";
    ss << "  </div>\n";
    ss << "</div>\n</body>\n</html>\n";
    
    m_content = ss.str();
}

// ── PDF Report Implementation (Architecture Skeleton) ───────────────────────
PdfReport::PdfReport(const MeasurementSession& session) {
    buildReport(session);
}

std::string PdfReport::getContent() const {
    return m_content;
}

void PdfReport::buildReport(const MeasurementSession& session) {
    // Architecture skeleton for PDF (will hold binary PDF bytes in future)
    std::stringstream ss;
    ss << "%PDF-1.4\n";
    ss << "% Placeholder for AMAS Generated PDF Report for session: " << session.sessionName << "\n";
    ss << "% Operator: " << session.metadata.operatorName << "\n";
    ss << "% Antenna: " << session.metadata.antennaModel << "\n";
    ss << "%%EOF\n";
    m_content = ss.str();
}

// ── Report Generator Factory ────────────────────────────────────────────────
std::shared_ptr<Report> ReportGenerator::generateReport(const MeasurementSession& session, ReportFormat format) {
    switch (format) {
        case ReportFormat::Markdown:
            return std::make_shared<MarkdownReport>(session);
        case ReportFormat::HTML:
            return std::make_shared<HtmlReport>(session);
        case ReportFormat::PDF:
            return std::make_shared<PdfReport>(session);
        default:
            return std::make_shared<MarkdownReport>(session);
    }
}

bool ReportGenerator::saveReportToFile(const std::shared_ptr<Report>& report, const std::string& filePath) {
    if (!report) {
        return false;
    }
    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Log::error("ReportGenerator: Failed to open file for writing: " + filePath);
        return false;
    }
    file << report->getContent();
    file.close();
    Log::info("ReportGenerator: Saved report file to: " + filePath);
    return true;
}

} // namespace AMAS
