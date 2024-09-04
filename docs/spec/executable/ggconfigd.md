# `ggconfigd` spec

`ggconfigd` creates a platform-agnostic interface to the configuration system
for Greengrass. `ggconfigd` provides snapshot, rollback, default loading, tlog
import/export and basic read/write access to the key/value store.

- [ggconfigd-1] `ggconfigd` shall maintain a key/value database of configuration
  data.
- [ggconfigd-2] `ggconfigd` shall provide corebus interfaces for read/write of
  configuration data.
- [ggconfigd-3] `ggconfigd` shall provide corebus interfaces for snapshots and
  rollback to support deployments.
- [ggconfigd-4] `ggconfigd` shall provide corebus interfaces for tlog
  import/export to support GG migration.
- [ggconfigd-5] `ggconfigd` shall be configured to ensure write operations are
  persisted in the event of unexpected reboots.
- [ggconfigd-6] `ggconfigd` shall provide corebus interfaces for IPC access of
  configuration data.
- _[ggconfigd-7] `ggconfigd` shall provide a mechanism to upgrade the datastore
  to newer versions_

## Data Model

The greengrass datamodel is a hierarchical key/value store. Keys are in the form
of paths: `root/path/key = value`. Keys are case insensitive (though they may be
stored internally with case) and consist of alphabetical characters separated by
a '/' spaces a numbers are not permitted in the keys.

> NOTE verify the key rules of greengrass legacy

Any data is permitted in a value. The data that goes in, is returned when read.

## Core Bus API

### write

Write a value to the indicated key. Create the key_path and key as required.

| Parameter name | Parameter Description                                                             |
| -------------- | --------------------------------------------------------------------------------- |
| component      | The name of the component. Used as the first entry in the key path                |
| key            | The key_path that is appended to the component name and addresses a piece of data |
| value          | The data value written to the component/key                                       |

### read

Read a value from the indicated key. If the key/key_path are not found, return
an error.

| Parameter name | Parameter Description                                                             |
| -------------- | --------------------------------------------------------------------------------- |
| component      | The name of the component. Used as the first entry in the key path                |
| key            | The key_path that is appended to the component name and addresses a piece of data |

> NOTE Can a partial path be requested that will return ALL the data below it?
> If so, what is the data format for this combined request?

### snapshot

Create a snapshot of the current configuration and store it with the current
date.

### restore

Restore a previous snapshot. If no date is provided, it will restore the
previous snapshot. If a date is provided then it must exactly match a previous
snapshot.

### snapshot_history

Return a list of previous snapshots and dates.

### export

Produce a TLOG export of the current configuration and save it to the specified
log file. A TLOG file is a combination of a complete dump of the entire
configuration and the delta to that configuration. For the export only the
complete dump is required.

### import

Import the specified log file, preferring the specified log file where there are
conflicts.

### merge

Merge the specified log file, preferring the newest data where there are
conflicts.

## IPC API

The following commands are a part of the IPC command set and supported by
ggconfigd.

### GetConfiguration

Gets a configuration value for a component on the core device. You specify the
key path for which to get a configuration value.

#### Parameters

> componentName (optional) : The name of the component. If no name is provided,
> the default is the callers name. key_path : The key_path to the configuration
> value. The key_path is a list where each entry in order is a value in the
> path. example: ["mqtt","port"] will return the value for `mqtt/port`. All
> values below the identified point in the key_path will be returned in a single
> `GglObject`. If the parameter is an empty list, all values for the componet
> will be returned.

#### Response

> componentName : The name of the component value : The requested configuration
> as an object.

### UpdateConfiguration

### SubscribeToConfigurationUpdate

## MERGE RULES

## KEY NAME RULES

### Error Constants

- ERRORS are part of the GGLITE Error handling.

| Error Name      | Purpose                                        |
| --------------- | ---------------------------------------------- |
| GGL_ERR_OK      | The command completed successfully             |
| GGL_ERR_FAILURE | The command failed. Check the logs for details |
| GGL_ERR_INVALID | The command parameters are incorrect           |

## Implementation

See [the ggconfigd design](../../design/ggconfigd.md).