#!/bin/bash

docker build $@ -t backpressure-test-clustered-provider:latest .

for image in `docker images | grep '<none' | awk '{print $3}'`
do
    docker rmi -f $image
done
