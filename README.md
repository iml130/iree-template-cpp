# IREE Template CPP

**DISCLAIMER**: This is a first working draft which still evolves, as also the CMake configuration in IREE is revised. It is not intended for everyday use and made available without any support.

This template demonstrates how to integrate [IREE](https://github.com/google/iree) into a third-party project with CMake. Runtime support is supported via the `simple_embedding` sample forked from IREE, where the original source can be found [here](https://github.com/google/iree/tree/master/iree/samples/simple_embedding). It planned to extend this by demonstrating how to build the `custom_dialect` sample, found [here](https://github.com/google/iree/tree/master/iree/samples/custom_dialect), out-of-tree.

The template is developed by Marius Brehler at Fraunhofer IML.
