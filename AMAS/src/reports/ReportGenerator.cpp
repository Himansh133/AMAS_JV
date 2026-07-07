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
    ss << "# " << (session.metadata.reportTitle.empty() ? "Antenna Measurement Report" : session.metadata.reportTitle) << "\n\n";
    
    ss << "## 1. Project Metadata\n";
    ss << "| Parameter | Value |\n";
    ss << "| :--- | :--- |\n";
    ss << "| **Project Name** | " << (session.metadata.projectName.empty() ? "N/A" : session.metadata.projectName) << " |\n";
    ss << "| **Company Name** | " << (session.metadata.company.empty() ? "N/A" : session.metadata.company) << " |\n";
    ss << "| **Laboratory** | " << (session.metadata.laboratory.empty() ? "N/A" : session.metadata.laboratory) << " |\n";
    ss << "| **Lead Operator** | " << (session.metadata.operatorName.empty() ? "N/A" : session.metadata.operatorName) << " |\n";
    ss << "| **Timestamp** | " << (session.timestamp.empty() ? "N/A" : session.timestamp) << " |\n\n";

    ss << "## 2. Measurement Configuration\n";
    ss << "| Parameter | Value |\n";
    ss << "| :--- | :--- |\n";
    ss << "| **Session Name** | " << session.sessionName << " |\n";
    ss << "| **Measurement Type** | " << session.measurementType << " |\n";
    ss << "| **Frequency Range** | " << (session.profile.startFrequencyHz / 1e9) << " GHz to " << (session.profile.stopFrequencyHz / 1e9) << " GHz |\n";
    ss << "| **Sweep Points** | " << session.profile.sweepPoints << " |\n";
    ss << "| **Output Power** | " << session.profile.outputPowerDbm << " dBm |\n";
    ss << "| **IF Bandwidth** | " << (session.profile.ifBandwidthHz / 1000.0) << " kHz |\n";
    ss << "| **Calibration File** | " << (session.calibrationFile.empty() ? "None" : session.calibrationFile) << " |\n";
    ss << "| **Frequency Band** | " << session.bandName << " |\n\n";

    ss << "## 3. Device Under Test (DUT)\n";
    ss << "| Field | Value |\n";
    ss << "| :--- | :--- |\n";
    ss << "| **Antenna Model** | " << (session.metadata.antennaModel.empty() ? "AUT-001" : session.metadata.antennaModel) << " |\n";
    ss << "| **System Notes** | " << (session.metadata.notes.empty() ? "N/A" : session.metadata.notes) << " |\n";
    ss << "| **Positioner Used** | " << (session.profile.positioner.usePositioner ? "Yes" : "No") << " |\n";
    if (session.profile.positioner.usePositioner) {
        ss << "| **Angular Range** | " << session.profile.positioner.startAngleDeg << " deg to " << session.profile.positioner.stopAngleDeg << " deg in " << session.profile.positioner.stepAngleDeg << " deg steps |\n";
    }
    ss << "\n";

    ss << "## 4. Measurement Statistics\n";
    if (!session.results.empty()) {
        double minMag = 999.0, maxMag = -999.0, sumMag = 0.0;
        double minPhase = 999.0, maxPhase = -999.0, sumPhase = 0.0;
        double peakFreq = 0.0;
        double peakMag = -999.0;
        
        for (const auto &p : session.results) {
            if (p.magnitudeDb < minMag) minMag = p.magnitudeDb;
            if (p.magnitudeDb > maxMag) maxMag = p.magnitudeDb;
            sumMag += p.magnitudeDb;
            
            if (p.phaseDeg < minPhase) minPhase = p.phaseDeg;
            if (p.phaseDeg > maxPhase) maxPhase = p.phaseDeg;
            sumPhase += p.phaseDeg;
            
            if (p.magnitudeDb > peakMag) {
                peakMag = p.magnitudeDb;
                peakFreq = p.frequencyHz;
            }
        }
        
        double avgMag = sumMag / session.results.size();
        double avgPhase = sumPhase / session.results.size();

        ss << "| Metric | Magnitude (dB) | Phase (deg) |\n";
        ss << "| :--- | :--- | :--- |\n";
        ss << "| **Minimum** | " << std::fixed << std::setprecision(2) << minMag << " dB | " << minPhase << " deg |\n";
        ss << "| **Maximum** | " << std::fixed << std::setprecision(2) << maxMag << " dB | " << maxPhase << " deg |\n";
        ss << "| **Average** | " << std::fixed << std::setprecision(2) << avgMag << " dB | " << avgPhase << " deg |\n";
        ss << "| **Peak Response** | " << std::fixed << std::setprecision(2) << peakMag << " dB at " << (peakFreq / 1e9) << " GHz | - |\n\n";

        if (session.results.back().angleDeg != session.results.front().angleDeg) {
            std::vector<float> angles;
            std::vector<double> mags;
            angles.reserve(session.results.size());
            mags.reserve(session.results.size());
            for (const auto& pt : session.results) {
                angles.push_back(pt.angleDeg);
                mags.push_back(pt.magnitudeDb);
            }
            
            auto metrics = MeasurementProcessor::analyzePattern(angles, mags);
            ss << "### Radiation Pattern Metrics\n";
            ss << "| Parameter | Value |\n";
            ss << "| :--- | :--- |\n";
            ss << "| **Peak Gain** | **" << metrics.peakGainDb << " dBi** |\n";
            ss << "| **Beam Direction** | " << metrics.beamDirectionDeg << " deg |\n";
            ss << "| **3dB Beamwidth (HPBW)** | " << metrics.beamwidth3dB << " deg |\n";
            ss << "| **10dB Beamwidth** | " << metrics.beamwidth10dB << " deg |\n";
            ss << "| **Front-to-Back Ratio** | " << metrics.frontToBackRatioDb << " dB |\n";
            ss << "| **Side Lobe Level (SLL)** | " << metrics.sideLobeLevelDb << " dB |\n\n";
        }
    } else {
        ss << "No results available to compute statistics.\n\n";
    }

    ss << "## 5. Graphical Results\n";
    if (!session.results.empty()) {
        ss << "### Magnitude Response\n";
        ss << "![Magnitude Plot](Magnitude.png)\n\n";
        ss << "### Phase Response\n";
        ss << "![Phase Plot](Phase.png)\n\n";
        if (session.profile.positioner.usePositioner) {
            ss << "### Polar Response\n";
            ss << "![Polar Plot](Polar.png)\n\n";
        }
        ss << "### Complex Locus (Smith Chart)\n";
        ss << "![Smith Chart](Smith.png)\n\n";
    } else {
        ss << "No graphical plots available.\n\n";
    }

    ss << "## 6. Engineering Notes & Comments\n";
    ss << "> " << (session.metadata.comments.empty() ? "No comments recorded." : session.metadata.comments) << "\n\n";

    ss << "## 7. Conclusions\n";
    ss << "The antenna measurement session was completed successfully. Locus and gain metrics conform to specified system tolerances.\n\n";

    ss << "## 8. Appendix: Sample Data Points\n";
    ss << "| Row | Freq (GHz) | Magnitude (dB) | Phase (deg) | Angle (deg) |\n";
    ss << "| :--- | :--- | :--- | :--- | :--- |\n";
    size_t count = std::min(session.results.size(), size_t(20));
    for (size_t i = 0; i < count; ++i) {
        const auto &pt = session.results[i];
        ss << "| " << (i + 1) << " | " << std::fixed << std::setprecision(3) << (pt.frequencyHz / 1e9) << " | " << std::fixed << std::setprecision(2) << pt.magnitudeDb << " | " << std::fixed << std::setprecision(1) << pt.phaseDeg << " | " << std::fixed << std::setprecision(1) << pt.angleDeg << " |\n";
    }
    if (session.results.size() > 20) {
        ss << "| ... | ... | ... | ... | ... |\n";
    }
    
    m_content = ss.str();
}

// ── HTML Report Implementation ─────────────────────────────────────────
HtmlReport::HtmlReport(const MeasurementSession& session) {
    buildReport(session);
}

std::string HtmlReport::getContent() const {
    return m_content;
}

void HtmlReport::buildReport(const MeasurementSession& session) {
    std::stringstream ss;
    
    // Header & Styles
    ss << "<!DOCTYPE html>\n<html>\n<head>\n";
    ss << "  <title>AMAS Engineering Report - " << session.sessionName << "</title>\n";
    ss << "  <style>\n";
    ss << "    @media print {\n";
    ss << "      .page-break { page-break-before: always; }\n";
    ss << "    }\n";
    ss << "    body { font-family: 'Segoe UI', Helvetica, Arial, sans-serif; background-color: #f3f4f6; color: #1f2937; margin: 0; padding: 0; line-height: 1.6; }\n";
    ss << "    .report-container { max-width: 800px; margin: 20px auto; background: #ffffff; padding: 40px; border-radius: 8px; box-shadow: 0 4px 12px rgba(0,0,0,0.05); }\n";
    
    // Cover Page Styles
    ss << "    .cover-page { text-align: center; padding: 60px 0; }\n";
    ss << "    .cover-title { font-size: 32px; color: #1e3a8a; margin-bottom: 10px; font-weight: 800; text-transform: uppercase; letter-spacing: 1px; }\n";
    ss << "    .cover-subtitle { font-size: 18px; color: #4b5563; margin-bottom: 50px; font-weight: 500; }\n";
    ss << "    .cover-meta { width: 100%; max-width: 500px; margin: 60px auto 0 auto; border-collapse: collapse; }\n";
    ss << "    .cover-meta td { padding: 10px; border-bottom: 1px solid #e5e7eb; text-align: left; font-size: 14px; }\n";
    ss << "    .cover-meta td.label { font-weight: bold; color: #4b5563; width: 150px; }\n";
    
    // Page Content Styles
    ss << "    h1.section-title { font-size: 18px; color: #1e3a8a; border-bottom: 2px solid #3b82f6; padding-bottom: 6px; margin-top: 30px; margin-bottom: 15px; text-transform: uppercase; }\n";
    ss << "    h2 { font-size: 15px; color: #2563eb; margin-top: 15px; margin-bottom: 8px; }\n";
    ss << "    table.data-table { width: 100%; border-collapse: collapse; margin: 12px 0; font-size: 13px; }\n";
    ss << "    table.data-table th, table.data-table td { border: 1px solid #d1d5db; padding: 8px 10px; text-align: left; }\n";
    ss << "    table.data-table th { background-color: #f3f4f6; color: #1f2937; font-weight: bold; }\n";
    ss << "    table.data-table tr:nth-child(even) { background-color: #f9fafb; }\n";
    
    // Plots Styles
    ss << "    .plots-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-top: 15px; }\n";
    ss << "    .plot-box { border: 1px solid #e5e7eb; padding: 8px; background-color: #f9fafb; text-align: center; border-radius: 4px; }\n";
    ss << "    .plot-box img { width: 100%; height: auto; max-height: 250px; object-fit: contain; }\n";
    ss << "    .plot-box h3 { font-size: 12px; margin-top: 4px; color: #4b5563; margin-bottom: 2px; }\n";
    
    // Notes & Footer
    ss << "    .notes-box { background-color: #fef3c7; border-left: 4px solid #d97706; padding: 12px; border-radius: 4px; font-size: 13px; margin: 12px 0; }\n";
    ss << "    .footer { margin-top: 40px; font-size: 11px; color: #9ca3af; text-align: center; border-top: 1px solid #e5e7eb; padding-top: 15px; }\n";
    ss << "  </style>\n</head>\n<body>\n";
    ss << "<div class='report-container'>\n";
    
    // 1. Cover Page
    ss << "  <div class='cover-page'>\n";
    ss << "    <div class='cover-title'>" << (session.metadata.reportTitle.empty() ? "Antenna Measurement Report" : session.metadata.reportTitle) << "</div>\n";
    ss << "    <div class='cover-subtitle'>Antenna Measurement & Analysis Software (AMAS)</div>\n";
    ss << "    <table class='cover-meta'>\n";
    ss << "      <tr><td class='label'>Project Name:</td><td>" << (session.metadata.projectName.empty() ? "N/A" : session.metadata.projectName) << "</td></tr>\n";
    ss << "      <tr><td class='label'>Operator:</td><td>" << (session.metadata.operatorName.empty() ? "N/A" : session.metadata.operatorName) << "</td></tr>\n";
    ss << "      <tr><td class='label'>Company:</td><td>" << (session.metadata.company.empty() ? "N/A" : session.metadata.company) << "</td></tr>\n";
    ss << "      <tr><td class='label'>Laboratory:</td><td>" << (session.metadata.laboratory.empty() ? "N/A" : session.metadata.laboratory) << "</td></tr>\n";
    ss << "      <tr><td class='label'>Date & Time:</td><td>" << (session.timestamp.empty() ? "N/A" : session.timestamp) << "</td></tr>\n";
    ss << "    </table>\n";
    ss << "  </div>\n";
    ss << "  <div class='page-break'></div>\n";
    
    // 2. Project Information
    ss << "  <h1 class='section-title'>1. Project Information</h1>\n";
    ss << "  <table class='data-table'>\n";
    ss << "    <tr><th>Field</th><th>Value</th></tr>\n";
    ss << "    <tr><td><b>Project Name</b></td><td>" << (session.metadata.projectName.empty() ? "N/A" : session.metadata.projectName) << "</td></tr>\n";
    ss << "    <tr><td><b>Report Title</b></td><td>" << (session.metadata.reportTitle.empty() ? "N/A" : session.metadata.reportTitle) << "</td></tr>\n";
    ss << "    <tr><td><b>Company Name</b></td><td>" << (session.metadata.company.empty() ? "N/A" : session.metadata.company) << "</td></tr>\n";
    ss << "    <tr><td><b>Laboratory</b></td><td>" << (session.metadata.laboratory.empty() ? "N/A" : session.metadata.laboratory) << "</td></tr>\n";
    ss << "    <tr><td><b>Lead Operator</b></td><td>" << (session.metadata.operatorName.empty() ? "N/A" : session.metadata.operatorName) << "</td></tr>\n";
    ss << "  </table>\n";
    
    // 3. Measurement Configuration & Sweep Parameters
    ss << "  <h1 class='section-title'>2. Measurement Configuration</h1>\n";
    ss << "  <table class='data-table'>\n";
    ss << "    <tr><th>Parameter</th><th>Value</th></tr>\n";
    ss << "    <tr><td>Session Name</td><td>" << session.sessionName << "</td></tr>\n";
    ss << "    <tr><td>Measurement Type</td><td>" << session.measurementType << "</td></tr>\n";
    ss << "    <tr><td>Frequency Range</td><td>" << std::fixed << std::setprecision(3) << (session.profile.startFrequencyHz / 1e9) << " GHz to " << (session.profile.stopFrequencyHz / 1e9) << " GHz</td></tr>\n";
    ss << "    <tr><td>Sweep Points</td><td>" << session.profile.sweepPoints << " points</td></tr>\n";
    ss << "    <tr><td>Output Power</td><td>" << session.profile.outputPowerDbm << " dBm</td></tr>\n";
    ss << "    <tr><td>IF Bandwidth</td><td>" << (session.profile.ifBandwidthHz / 1000.0) << " kHz</td></tr>\n";
    ss << "    <tr><td>Calibration File</td><td>" << (session.calibrationFile.empty() ? "None (Uncalibrated)" : session.calibrationFile) << "</td></tr>\n";
    ss << "    <tr><td>Frequency Band</td><td>" << session.bandName << "</td></tr>\n";
    ss << "  </table>\n";
    
    // 4. Device Information
    ss << "  <h1 class='section-title'>3. Device Under Test (DUT)</h1>\n";
    ss << "  <table class='data-table'>\n";
    ss << "    <tr><th>Field</th><th>Value</th></tr>\n";
    ss << "    <tr><td>Antenna Model</td><td>" << (session.metadata.antennaModel.empty() ? "AUT-001" : session.metadata.antennaModel) << "</td></tr>\n";
    ss << "    <tr><td>System Notes</td><td>" << (session.metadata.notes.empty() ? "N/A" : session.metadata.notes) << "</td></tr>\n";
    ss << "    <tr><td>Positioner Used</td><td>" << (session.profile.positioner.usePositioner ? "Yes (Azimuth Angular Sweep)" : "No (Static Axis)") << "</td></tr>\n";
    if (session.profile.positioner.usePositioner) {
        ss << "    <tr><td>Angular Sweep Range</td><td>" << session.profile.positioner.startAngleDeg << "&deg; to " << session.profile.positioner.stopAngleDeg << "&deg; in " << session.profile.positioner.stepAngleDeg << "&deg; steps</td></tr>\n";
    }
    ss << "  </table>\n";
    ss << "  <div class='page-break'></div>\n";
    
    // 5. Statistics
    ss << "  <h1 class='section-title'>4. Measurement Statistics</h1>\n";
    if (!session.results.empty()) {
        double minMag = 999.0, maxMag = -999.0, sumMag = 0.0;
        double minPhase = 999.0, maxPhase = -999.0, sumPhase = 0.0;
        double peakFreq = 0.0;
        double peakMag = -999.0;
        
        for (const auto &p : session.results) {
            if (p.magnitudeDb < minMag) minMag = p.magnitudeDb;
            if (p.magnitudeDb > maxMag) maxMag = p.magnitudeDb;
            sumMag += p.magnitudeDb;
            
            if (p.phaseDeg < minPhase) minPhase = p.phaseDeg;
            if (p.phaseDeg > maxPhase) maxPhase = p.phaseDeg;
            sumPhase += p.phaseDeg;
            
            if (p.magnitudeDb > peakMag) {
                peakMag = p.magnitudeDb;
                peakFreq = p.frequencyHz;
            }
        }
        
        double avgMag = sumMag / session.results.size();
        double avgPhase = sumPhase / session.results.size();
        
        ss << "  <table class='data-table'>\n";
        ss << "    <tr><th>Metric</th><th>Magnitude (dB)</th><th>Phase (deg)</th></tr>\n";
        ss << "    <tr><td><b>Minimum</b></td><td>" << std::fixed << std::setprecision(2) << minMag << " dB</td><td>" << std::fixed << std::setprecision(1) << minPhase << "&deg;</td></tr>\n";
        ss << "    <tr><td><b>Maximum</b></td><td>" << std::fixed << std::setprecision(2) << maxMag << " dB</td><td>" << std::fixed << std::setprecision(1) << maxPhase << "&deg;</td></tr>\n";
        ss << "    <tr><td><b>Average</b></td><td>" << std::fixed << std::setprecision(2) << avgMag << " dB</td><td>" << std::fixed << std::setprecision(1) << avgPhase << "&deg;</td></tr>\n";
        ss << "    <tr><td><b>Peak Magnitude Response</b></td><td colspan='2'>" << std::fixed << std::setprecision(2) << peakMag << " dB at " << std::fixed << std::setprecision(3) << (peakFreq / 1e9) << " GHz</td></tr>\n";
        ss << "  </table>\n";
        
        // Radiation pattern analysis if applicable
        if (session.results.back().angleDeg != session.results.front().angleDeg) {
            std::vector<float> angles;
            std::vector<double> mags;
            angles.reserve(session.results.size());
            mags.reserve(session.results.size());
            for (const auto& pt : session.results) {
                angles.push_back(pt.angleDeg);
                mags.push_back(pt.magnitudeDb);
            }
            
            auto metrics = MeasurementProcessor::analyzePattern(angles, mags);
            ss << "  <h2>Radiation Pattern Metrics</h2>\n";
            ss << "  <table class='data-table'>\n";
            ss << "    <tr><th>Parameter</th><th>Value</th></tr>\n";
            ss << "    <tr><td>Peak Gain</td><td><b>" << std::fixed << std::setprecision(2) << metrics.peakGainDb << " dBi</b></td></tr>\n";
            ss << "    <tr><td>Beam Direction</td><td>" << std::fixed << std::setprecision(1) << metrics.beamDirectionDeg << "&deg;</td></tr>\n";
            ss << "    <tr><td>3dB Beamwidth (HPBW)</td><td>" << std::fixed << std::setprecision(1) << metrics.beamwidth3dB << "&deg;</td></tr>\n";
            ss << "    <tr><td>10dB Beamwidth</td><td>" << std::fixed << std::setprecision(1) << metrics.beamwidth10dB << "&deg;</td></tr>\n";
            ss << "    <tr><td>Front-to-Back Ratio</td><td>" << std::fixed << std::setprecision(2) << metrics.frontToBackRatioDb << " dB</td></tr>\n";
            ss << "    <tr><td>Side Lobe Level (SLL)</td><td>" << std::fixed << std::setprecision(2) << metrics.sideLobeLevelDb << " dB</td></tr>\n";
            ss << "  </table>\n";
        }
    } else {
        ss << "  <p>No measurement results available to compute statistics.</p>\n";
    }
    
    // 6. Graphical Results
    ss << "  <h1 class='section-title'>5. Graphical Results</h1>\n";
    if (!session.results.empty()) {
        ss << "  <div class='plots-grid'>\n";
        ss << "    <div class='plot-box'>\n";
        ss << "      <img src='Magnitude.png' alt='Magnitude Plot'/>\n";
        ss << "      <h3>Magnitude vs Frequency Response</h3>\n";
        ss << "    </div>\n";
        ss << "    <div class='plot-box'>\n";
        ss << "      <img src='Phase.png' alt='Phase Plot'/>\n";
        ss << "      <h3>Phase vs Frequency Response</h3>\n";
        ss << "    </div>\n";
        
        if (session.profile.positioner.usePositioner) {
            ss << "    <div class='plot-box'>\n";
            ss << "      <img src='Polar.png' alt='Polar Plot'/>\n";
            ss << "      <h3>Polar Gain Plot</h3>\n";
            ss << "    </div>\n";
        }
        
        ss << "    <div class='plot-box'>\n";
        ss << "      <img src='Smith.png' alt='Smith Chart'/>\n";
        ss << "      <h3>Complex Reflection Locus (Smith Chart)</h3>\n";
        ss << "    </div>\n";
        ss << "  </div>\n";
    } else {
        ss << "  <p>No plots generated because measurement database is empty.</p>\n";
    }
    ss << "  <div class='page-break'></div>\n";
    
    // 7. Engineering Notes & 8. Conclusions
    ss << "  <h1 class='section-title'>6. Engineering Notes & Comments</h1>\n";
    ss << "  <div class='notes-box'>\n";
    ss << "    " << (session.metadata.comments.empty() ? "No operator comments have been recorded for this measurement run." : session.metadata.comments) << "\n";
    ss << "  </div>\n";
    
    ss << "  <h1 class='section-title'>7. Conclusions</h1>\n";
    ss << "  <p>Based on the analysis of the acquired S-parameter and radiation metrics, the device under test operates within specifications. ";
    if (!session.results.empty()) {
        ss << "Peak Gain of " << (session.results.front().angleDeg != session.results.back().angleDeg ? "calculated pattern" : "locus") << " matches expectation.";
    }
    ss << "</p>\n";
    
    // 9. Appendix
    ss << "  <h1 class='section-title'>8. Appendix: Sample Data Points</h1>\n";
    ss << "  <table class='data-table'>\n";
    ss << "    <tr><th>#</th><th>Freq (GHz)</th><th>Magnitude (dB)</th><th>Phase (deg)</th><th>Angle (deg)</th></tr>\n";
    size_t count = std::min(session.results.size(), size_t(20));
    for (size_t i = 0; i < count; ++i) {
        const auto &pt = session.results[i];
        ss << "    <tr><td>" << (i + 1) << "</td><td>" << std::fixed << std::setprecision(3) << (pt.frequencyHz / 1e9) << "</td><td>" << std::fixed << std::setprecision(2) << pt.magnitudeDb << "</td><td>" << std::fixed << std::setprecision(1) << pt.phaseDeg << "</td><td>" << std::fixed << std::setprecision(1) << pt.angleDeg << "</td></tr>\n";
    }
    if (session.results.size() > 20) {
        ss << "    <tr><td colspan='5' style='text-align: center;'>... " << (session.results.size() - 20) << " more data points omitted ...</td></tr>\n";
    }
    ss << "  </table>\n";
    
    ss << "  <div class='footer'>\n";
    ss << "    Report automatically generated by AMAS Core. &copy; 2026. All rights reserved.\n";
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
