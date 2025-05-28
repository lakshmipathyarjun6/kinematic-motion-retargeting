#ifndef CONTACTTRANSFEREDITCONTEXTCOMMAND_H
#define CONTACTTRANSFEREDITCONTEXTCOMMAND_H

#include <maya/MPxContextCommand.h>

#include "contactTransferEditContext.hpp"

// Transfer flags

#define JUMP_FLAG "-j"
#define JUMP_FLAG_LONG "-jump"

#define TRANSFER_CONTACTS_FLAG "-tc"
#define TRANSFER_CONTACTS_FLAG_LONG "-transfercontacts"

#define TRANSFER_CONTACTS_BULK_FLAG "-btc"
#define TRANSFER_CONTACTS_BULK_FLAG_LONG "-bulktransfercontacts"

// Filter flags

#define COMMIT_PARAMETERIZED_DISTANCES_FLAG "-cpd"
#define COMMIT_PARAMETERIZED_DISTANCES_FLAG_LONG "-commitparameterizeddistances"

#define DUMP_PARAMETERIZED_DISTANCES_FLAG "-dpd"
#define DUMP_PARAMETERIZED_DISTANCES_FLAG_LONG "-dumpparameterizeddistances"

#define PARAMETERIZED_DISTANCE_THRESHOLD_FLAG "-pfd"
#define PARAMETERIZED_DISTANCE_THRESHOLD_FLAG_LONG                             \
    "-parameterizedfilterdistance"

class ContactTransferEditContextCommand : public MPxContextCommand
{
public:
    ContactTransferEditContextCommand();
    virtual ~ContactTransferEditContextCommand();
    static void *creator();

    virtual MStatus appendSyntax();
    virtual MStatus doEditFlags();
    virtual MPxContext *makeObj();

protected:
    ContactTransferEditContext *m_pContext;
};

#endif
