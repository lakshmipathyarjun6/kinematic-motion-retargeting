#ifndef CONTACTRAYTRACECONTEXTCOMMAND_H
#define CONTACTRAYTRACECONTEXTCOMMAND_H

#include <maya/MPxContextCommand.h>

#include "contactRaytraceContext.hpp"

// Primary flags

#define DUMP_CONTACT_DISTANCES_FLAG "-dcd"
#define DUMP_CONTACT_DISTANCES_FLAG_LONG "-dumpcontactdistances"

#define JUMP_FLAG "-j"
#define JUMP_FLAG_LONG "-jump"

#define PURGE_CONTACTS_FLAG "-pc"
#define PURGE_CONTACTS_FLAG_LONG "-purgecontacts"

#define RAYTRACE_CONTACTS_FLAG "-rc"
#define RAYTRACE_CONTACTS_FLAG_LONG "-raytracecontacts"

#define RAYTRACE_CONTACTS_BULK_FLAG "-brc"
#define RAYTRACE_CONTACTS_BULK_FLAG_LONG "-bulkraytracecontacts"

class ContactRaytraceContextCommand : public MPxContextCommand
{
public:
    ContactRaytraceContextCommand();
    virtual ~ContactRaytraceContextCommand();
    static void *creator();

    virtual MStatus appendSyntax();
    virtual MStatus doEditFlags();
    virtual MPxContext *makeObj();

protected:
    ContactRaytraceContext *m_pContext;
};

#endif
