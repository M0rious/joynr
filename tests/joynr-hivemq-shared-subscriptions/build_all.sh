#!/bin/bash

for BUILD_DIR in test-apps/backpressure-clustered-provider-large test-apps/backpressure-clustered-provider-small test-apps/backpressure-monitor-app test-apps/clustered-app test-apps/monitor-app test-driver-container
do
	pushd $BUILD_DIR
	./build_docker_image.sh $@
	popd
done
