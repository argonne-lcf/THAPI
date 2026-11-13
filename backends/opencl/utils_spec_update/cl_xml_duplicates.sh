#!/usr/bin/env bash
# Usage: utils_spec_update/cl_xml_duplicates.sh <cl.xml>
#
# Prints the names a cl.xml defines more than once: types, commands, enum
# groups and enum values. References (<type name="..."/>, <enum name="..."/>
# in a <require>) and XML comments (cl.xml keeps some retired values in
# <!-- -->) do not count.
xml=$(perl -0pe 's/<!--.*?-->//gs' "$1")
defs() {
  grep -oE '<type [^>]*category="[^"]*"[^>]*>.*<name>[A-Za-z_0-9]+</name>' <<<"$xml" | sed -E 's/.*<name>([^<]+)<\/name>$/type \1/'
  grep -oE '<proto>.*<name>[A-Za-z_0-9]+</name></proto>' <<<"$xml" | sed -E 's/.*<name>([^<]+)<\/name><\/proto>/command \1/'
  grep -oE '<enums name="[A-Za-z_0-9]+"' <<<"$xml" | sed -E 's/<enums name="(.*)"/enums \1/'
  grep -oE '<enum value="[^"]*" +name="[A-Za-z_0-9]+"' <<<"$xml" | sed -E 's/.*name="(.*)"/enum \1/'
}
defs | sort | uniq -d
