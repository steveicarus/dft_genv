#!/bin/sh

# This script generates the BUILD.v source file that is needed
# to supply the build details to the Verilog design. Run this
# in front of any compilation to generate the build id that
# is then compiled into the design.

# The build id is a simple date stamp.
BUILD_ID=`date -u +32\'d%Y%m%d%H`

cat <<EOF
\`timescale 1ns/1ns
\`default_nettype none
// AUTOMATICALLY GENERATED -- DO NOT EDIT
module BUILD (output wire [31:0] build_id);
   assign build_id = $BUILD_ID;
endmodule
EOF

