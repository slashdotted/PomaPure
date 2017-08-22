# PoorMan's Pipeline

This tool enables you to easily create modular pipelines for processing data. Multiple threads and/or distributed execution (on multiple machines) is natively supported. Pipelines are defined through a JSON file: each module is compiled into a separate library which gets loaded dynamically. If necessary pipeline descriptions can be embedded into the code of your application.
A description of the base modules shipped with this tool is in the Documentation directory.

To compile you need to satisfy the following dependencies:

- Boost >= 1.63.0
- (for distributed processing) 0MQ
