#ifndef AMAS_REPORTGENERATOR_H
#define AMAS_REPORTGENERATOR_H

#include <string>
#include <memory>
#include "measurement/MeasurementSession.h"

namespace AMAS {

enum class ReportFormat {
    PDF,
    HTML,
    Markdown
};

class Report {
public:
    virtual ~Report() = default;
    virtual std::string getContent() const = 0;
    virtual ReportFormat getFormat() const = 0;
};

class MarkdownReport : public Report {
public:
    explicit MarkdownReport(const MeasurementSession& session);
    std::string getContent() const override;
    ReportFormat getFormat() const override { return ReportFormat::Markdown; }
private:
    std::string m_content;
    void buildReport(const MeasurementSession& session);
};

class HtmlReport : public Report {
public:
    explicit HtmlReport(const MeasurementSession& session);
    std::string getContent() const override;
    ReportFormat getFormat() const override { return ReportFormat::HTML; }
private:
    std::string m_content;
    void buildReport(const MeasurementSession& session);
};

class PdfReport : public Report {
public:
    explicit PdfReport(const MeasurementSession& session);
    std::string getContent() const override;
    ReportFormat getFormat() const override { return ReportFormat::PDF; }
private:
    std::string m_content;
    void buildReport(const MeasurementSession& session);
};

class ReportGenerator {
public:
    // Factory method to produce a report in the specified format
    static std::shared_ptr<Report> generateReport(const MeasurementSession& session, ReportFormat format);

    // Save the report content to the disk
    static bool saveReportToFile(const std::shared_ptr<Report>& report, const std::string& filePath);
};

} // namespace AMAS

#endif // AMAS_REPORTGENERATOR_H
