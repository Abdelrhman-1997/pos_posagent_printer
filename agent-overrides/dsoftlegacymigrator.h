#ifndef DSOFT_LEGACYMIGRATOR_H
#define DSOFT_LEGACYMIGRATOR_H

#include <QString>

class PrinterProfileManager;

class DSoftLegacyMigrator {
public:
  static bool migrateIfNeeded(PrinterProfileManager *manager,
                              QString *message = nullptr);
};

#endif // DSOFT_LEGACYMIGRATOR_H
