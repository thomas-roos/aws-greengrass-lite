#include "config.h"

//
// TODO: Decide if this belongs in Nucleus, or belongs in another plugin
// Note that config intake is case insensitive - config comes from
// a settings file (YAML), transaction log (YAML), or cloud (JSON or YAML)
// For optimization, this implementation assumes all config keys are stored lower-case
// which means translation on intake is important
//
