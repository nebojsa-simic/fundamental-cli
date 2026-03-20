---
name: fundamental-config
description: Configuration with Fundamental Library - load, cascade, get values
license: MIT
compatibility: Complements fundamental-expert skill
metadata:
  author: fundamental-library
  version: "1.0"
  category: configuration
  related: fundamental-console, fundamental-string
---

# Fundamental Library - Configuration Skill

I provide copy-paste examples for configuration management using the Fundamental Library.

---

## Quick Reference

| Task | Function | Example |
|------|----------|---------|
| Load config | `fun_config_load()` | See below |
| Get string | `fun_config_get_string()` | See below |
| Get int | `fun_config_get_int()` | See below |
| Get bool | `fun_config_get_bool()` | See below |
| Get with default | `fun_config_get_*_or_default()` | See below |

**Cascade Priority (highest to lowest):**
1. CLI arguments (`--config:key=value`)
2. Environment variables (`APPNAME_KEY=value`)
3. INI file (`~/.config/appname/appname.ini`)

---

## Task: Load Configuration

Load configuration with cascading sources.

```c
#include "config/config.h"
#include "console/console.h"

int config_load_example(void)
{
    // Load config (cascade: CLI → env → INI)
    ConfigResult config_result = fun_config_load("myapp");
    if (fun_error_is_error(config_result.error)) {
        fun_console_error_line("Failed to load configuration");
        return 1;
    }
    Config *config = &config_result.value;
    
    // Use config...
    
    // Cleanup
    fun_config_destroy(config);
    return 0;
}
```

**What gets loaded:**
- CLI: `./myapp --config:database.host=localhost`
- Env: `MYAPP_DATABASE_HOST=localhost`
- INI: `~/.config/myapp/myapp.ini`

---

## Task: Get Required String Value

Get a required string configuration value.

```c
#include "config/config.h"
#include "console/console.h"

void required_string_example(Config *config)
{
    // Required value - explicit error check
    StringResult host_result = fun_config_get_string(config, "database.host");
    
    if (fun_error_is_error(host_result.error)) {
        fun_console_error_line("database.host is required");
        return;  // or exit
    }
    
    const char *host = host_result.value;
    fun_console_write("Connecting to: ");
    fun_console_write_line(host);
}
```

**Key Points:**
- Required values should have explicit error handling
- Inform user which required value is missing
- Consider exiting if required config is absent

---

## Task: Get Optional Value with Default

Get an optional value with a sensible default.

```c
#include "config/config.h"

void optional_with_default_example(Config *config)
{
    // Optional int with default
    int64_t port = fun_config_get_int_or_default(
        config, "database.port", 5432);
    
    // Optional bool with default
    bool debug = fun_config_get_bool_or_default(
        config, "debug.enabled", false);
    
    // Optional string with default
    StringResult log_level_result = fun_config_get_string_or_default(
        config, "logging.level", "info");
    const char *log_level = log_level_result.value;
}
```

**Common Defaults:**
```c
int64_t timeout = fun_config_get_int_or_default(config, "timeout", 30);
bool ssl = fun_config_get_bool_or_default(config, "ssl.enabled", true);
int64_t max_connections = fun_config_get_int_or_default(config, "pool.max", 10);
```

---

## Task: Config Cascade Explained

Understanding the cascade priority.

```ini
# File: ~/.config/myapp/myapp.ini
[database]
host = localhost
port = 5432

[debug]
enabled = false
```

```bash
# Environment (overrides INI)
export MYAPP_DATABASE_HOST=production.db.example.com

# CLI (overrides everything)
./myapp --config:database.host=cli-override --config:debug.enabled=true
```

**Final values:**
- `database.host` = `cli-override` (from CLI)
- `database.port` = `5432` (from INI)
- `debug.enabled` = `true` (from CLI)

**Code to read:**
```c
StringResult host = fun_config_get_string(config, "database.host");
// Returns "cli-override"

int64_t port = fun_config_get_int_or_default(config, "database.port", 3306);
// Returns 5432

bool debug = fun_config_get_bool_or_default(config, "debug.enabled", false);
// Returns true
```

---

## Task: Get Different Value Types

Get various configuration value types.

```c
#include "config/config.h"

void get_all_types_example(Config *config)
{
    // String
    StringResult str = fun_config_get_string(config, "database.host");
    if (fun_error_is_ok(str.error)) {
        const char *host = str.value;
    }
    
    // Integer
    int64_t port = fun_config_get_int_or_default(config, "database.port", 5432);
    
    // Boolean (true/false, yes/no, 1/0)
    bool debug = fun_config_get_bool_or_default(config, "debug", false);
    
    // Double
    // (if available in your version)
    // double threshold = fun_config_get_double_or_default(config, "threshold", 0.5);
}
```

**Boolean Parsing:**
- `true`, `yes`, `1` → `true`
- `false`, `no`, `0` → `false`

---

## Task: Nested Configuration Keys

Access nested configuration sections.

```ini
# INI file format
[database]
host = localhost
port = 5432

[database.connection]
timeout = 30
retries = 3

[logging]
level = info
file = /var/log/myapp.log
```

```c
// Access with dot notation
StringResult host = fun_config_get_string(config, "database.host");
int64_t port = fun_config_get_int_or_default(config, "database.port", 5432);
int64_t timeout = fun_config_get_int_or_default(config, "database.connection.timeout", 30);
int64_t retries = fun_config_get_int_or_default(config, "database.connection.retries", 3);
StringResult level = fun_config_get_string(config, "logging.level");
```

---

## Complete Example

```c
#include "config/config.h"
#include "console/console.h"
#include "memory/memory.h"
#include "string/string.h"

int main(int argc, const char **argv)
{
    // Load configuration
    ConfigResult config_result = fun_config_load("myapp");
    if (fun_error_is_error(config_result.error)) {
        fun_console_error_line("Failed to load configuration");
        return 1;
    }
    Config *config = &config_result.value;
    
    // Required config - must exist
    StringResult host_result = fun_config_get_string(config, "database.host");
    if (fun_error_is_error(host_result.error)) {
        fun_console_error_line("ERROR: database.host is required");
        fun_console_error_line("Set via: --config:database.host=VALUE");
        fun_console_error_line("     or: export MYAPP_DATABASE_HOST=VALUE");
        fun_console_error_line("     or: Add [database] host=VALUE to myapp.ini");
        fun_config_destroy(config);
        return 1;
    }
    
    // Optional config with defaults
    int64_t port = fun_config_get_int_or_default(config, "database.port", 5432);
    bool debug = fun_config_get_bool_or_default(config, "debug.enabled", false);
    int64_t timeout = fun_config_get_int_or_default(config, "database.timeout", 30);
    
    // Build connection string
    MemoryResult mem_result = fun_memory_allocate(256);
    if (fun_error_is_error(mem_result.error)) {
        fun_config_destroy(config);
        return 1;
    }
    char *connection_string = (char *)mem_result.value;
    
    StringTemplateParam params[] = {
        { "host", { .stringValue = host_result.value } },
        { "port", { .intValue = port } }
    };
    
    fun_string_template(
        "postgresql://${host}:#{port}/main?timeout=#{timeout}",
        params,
        3,
        connection_string
    );
    
    if (debug) {
        fun_console_write_line("Connecting with: ");
        fun_console_write_line(connection_string);
    }
    
    // Use connection_string...
    
    // Cleanup
    fun_memory_free(&mem_result.value);
    fun_config_destroy(config);
    
    return 0;
}
```

---

## See Also

- **[fundamental-string.md](fundamental-string.md)** - String templating
- **[fundamental-console.md](fundamental-console.md)** - Output for errors
- **[config/config.h](../../include/config/config.h)** - Complete config API
