#!/usr/bin/awk -f

# Convert a text file exported by vym to a standard indented text format.
# It is not as pleasant to read, but much better for data interchange,
# like pasting into Freeplane.

BEGIN {
   # Each record is a paragraph; each field is a line
   RS=""; FS="\n"; ORS="\n\n"
}

# Top or 1st level heading is followed by a line of "="
$2 ~ /^=+$/ {
   printf("%s\n", $1)
   next
}

# 2nd level heading is followed by a line of "-"
$2 ~ /^-+$/ {
   # remove number prefix like "88. "
   sub(/^[0-9\. ]*/, "", $1)  
   printf("  %s\n", $1)
   next
}

1 {
   for (i=1; i<=NF; i++) {
      # remove prefix like "* " or "- "
      sub(/[*-] /, "", $i)
      printf("  %s\n", $i)
   }
}

