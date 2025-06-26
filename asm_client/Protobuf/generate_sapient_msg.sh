#!/bin/sh
google/protobuf/protoc --cpp_out=. sapient_msg/proto_options.proto sapient_msg/bsi_flex_335_v2_0/*.proto
