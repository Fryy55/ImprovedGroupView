# Improved Group View

Allows more than 20 groups to show in the groups menu!

# For developers

This mod exposes an API that allows you to post events to refresh the new group menu. It works both as a `required` and an optional (`recommended`/`suggested`) dependency.

## Usage:

In your source file, put\
`#include <alphalaneous.improved_group_view/api/GroupViewUpdateEvent.hpp>`

Then, whenever you want to update the group view just call\
`igv::GroupViewUpdateEvent().post()`