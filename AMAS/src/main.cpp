#include "Application.h"
#include <QApplication>

#include "theme/ThemeManager.h"

using namespace AMAS;

int main(int argc, char *argv[]) {
  // Initialize standard Qt Application with GUI capabilities
  QApplication a(argc, argv);

  // Apply visual theme globally
  ThemeManager::applyTheme(&a);

  Application app;
  if (!app.initialize()) {
    return 1;
  }
  app.run();

  // Execute Qt Main Event Loop
  return a.exec();
}
