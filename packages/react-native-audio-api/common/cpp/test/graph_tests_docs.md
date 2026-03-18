
Here is the guide on how to run the graph tests locally with some tips and tricks to make it easier.


## Running it normally
```
yarn test:graph
```
> This is most common way to run the tests, but it has subtle difference on macos

## Running on docker
```
yarn test:graph:docker
```
> This way we ensure that the test enviroment is simmilar to the one used in CI, it is much slower and requires docker to be installed


## Tips and tricks
As running docker tests takes forevewer it is recommended to relly on CI/CD. Tests without docker does not have ASAN and may not catch memory leaks or address realated issues. So if any of these issues occur in CI/CD here is how you can run single test with docker to debug it locally:
add the following line to the `RunTestsGraphDocker.sh` file in docker command:

```bash
docker run --rm -it \
  --name $CONTAINER_NAME \
  -v "$REPO_ROOT:/workspace" \
  -w /workspace/packages/react-native-audio-api/common/cpp/test \
  -e ASAN_OPTIONS=detect_leaks=1:verbosity=2 \
  -e GRAPH_FILTER="<your_test_filter_here>" \ # Add this line to set the filter for the tests
  $IMAGE_NAME \
  bash RunTestsGraph.sh
```

This will add enviromental variable `GRAPH_FILTER` which will limit the tests cases to only ones matching the filter. This way you can run single test case or a group instead of running all the tests all the time.
