# IREE Template CPP

**DISCLAIMER**: This is a first working draft which still evolves, as also the CMake configuration in IREE is revised. It is not intended for everyday use and made available without any support.

This template demonstrates how to integrate [IREE](https://github.com/google/iree) into a third-party project with CMake.
IREE runtime support is demonstrated via the `simple_embedding` sample forked from IREE, where the original source can be found [here](https://github.com/google/iree/tree/master/iree/samples/simple_embedding).
The `custom_modules` sample further demonstrates the out-of-tree usage of a custom dialect alongside with the runtime. The original `custom_modules` source can found [here](https://github.com/google/iree/tree/master/iree/samples/custom_dialect).

The template is developed by Marius Brehler at Fraunhofer IML.
