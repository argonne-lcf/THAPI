#!/usr/bin/env bash
# Usage: utils_spec_update/cl_xml_duplicates.sh <cl.xml>
#
# Prints the names a cl.xml defines more than once: types, commands, enum
# groups and enum values. References (<type name="..."/>, <enum name="..."/>
# in a <require>) do not count.
defs() {
  grep -oE '<type [^>]*category="[^"]*"[^>]*>.*<name>[A-Za-z_0-9]+</name>' "$1" | sed -E 's/.*<name>([^<]+)<\/name>$/type \1/'
  grep -oE '<proto>.*<name>[A-Za-z_0-9]+</name></proto>' "$1" | sed -E 's/.*<name>([^<]+)<\/name><\/proto>/command \1/'
  grep -oE '<enums name="[A-Za-z_0-9]+"' "$1" | sed -E 's/<enums name="(.*)"/enums \1/'
  grep -oE '<enum value="[^"]*" +name="[A-Za-z_0-9]+"' "$1" | sed -E 's/.*name="(.*)"/enum \1/'
}
defs "$1" | sort | uniq -d
