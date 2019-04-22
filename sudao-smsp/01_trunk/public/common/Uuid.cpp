#include "Uuid.h"

string getUUID()
{
    uuid_t uuid;
    char str[36];

    uuid_generate(uuid);
    uuid_unparse(uuid, str);

    return string(str);
}
