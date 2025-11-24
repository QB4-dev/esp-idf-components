# esp-tzdata

Timezone data component for ESP-IDF.

## Overview

`esp-tzdata` provides a small, embedded timezone database (tzdata) and a
convenience API to set the system timezone at runtime using names such as
`Europe/Warsaw` or `America/New_York`.

## Installation

### Using ESP Component Registry

[![Component Registry](https://components.espressif.com/components/qb4-dev/tzdata/badge.svg)](https://components.espressif.com/components/qb4-dev/tzdata)

```bash
idf.py add-dependency "qb4-dev/tzdata=*"
```

### Manual Installation

Clone this repository into your project's `components` directory:

```bash
cd your_project/components
git clone https://github.com/QB4-dev/esp-tzdata.git tzdata
```

## Usage

Include the header and call the API to set the timezone by name:

```c
#include "timezone.h"

// ...
if (tzdata_set_timezone("Europe/Warsaw") == ESP_OK) {
		// timezone applied
}
```

The timezone names available are defined in `tz_data.txt`. Each entry maps a
human-readable timezone name to a POSIX TZ string.

## Updating tzdata

- `tz_data.txt` contains the list of timezone name / POSIX TZ pairs used to
	generate the `tz_data` table at compile time. Update that file to add or
	remove entries and rebuild the project.

