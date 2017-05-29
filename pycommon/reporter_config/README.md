Reporter Configuration
==========

This Python module serves as an improved version of reporting modules of the NEMEA system. The work is based on modification and improvement of a shared python module called *report2idea*.

The goal of the new version is to enable advanced alert filtering that is performed according to global configuration file. Besides just filtering, users can specify a list of *actions* that are executed when the given *condition* is fulfilled and a list of *elseactions* that are executed when the *condition* is not fulfilled.

This document describes the analysis and implementation of such expansion while defining a format for configuration which is the cornerstone of the whole system.

Analysis
========

System Architecture
-------------------

The current reporters’ architecture is modular and easily expandable. Each reporter module imports common Python module called *report2idea* that implements core functionality of handling reported alerts. *report2idea* connects to the TRAP[1] interface, receives alerts and performs desired actions. Currently it allows for storing IDEA formatted alerts into MongoDB, Warden system and resending them via TRAP interface. These options depend on arguments passed to the reporter on start up.

The new architecture expands *report2idea* module where on startup a YAML configuration file is loaded, checked and parsed by *report\_config* module. When an alert is received it’s matched against rules specified in the configuration and else/actions are performed (more about this in section ).

*reporter_config* uses Mentat Filtering[2] module to parse conditions in rules and match alerts against them. Mentat internally uses its own module *jpath* to traverse paths in dictionaries which is used in conditions to retrieve values from an alert.

Configuration Design
--------------------

### Rules

Each rule is composed of *ID*, *condition*, *actions* and *elseactions* which is an optional item. The list of rules is ordered by its numeric ID and evaluated in that order.

Filtering condition consists of unary and binary operations, constants and JSON paths that are parsed by the Mentat Filtering module. When an alert meets the condition, list of actions is performed in specified order. If a list of elseactions is defined and the filtering condition is not met this list of actions is performed.

### Actions

Each rule specifies a list of *actions* and optionally *elseactions* that are of two types: implicit and custom. Currently the only implicit action available is to `drop` the alert which will immidiatelly stop processing it. This action mustn’t be specified in custom actions.

The `custom_actions` is a list of actions identified by ID string key – this identifier is used to reference an else/action in a rule. Each custom action must specify its type. Currently available types of custom actions are as follows:

-   <span>file – store alert into file</span>

    -   <span>dir\[boolean\] – if set to `False` each alert is stored in separate file, otherwise it is appended to specified file</span>

    -   <span>path\[string\] – path to file/folder respectively to `dir` option</span>

-   <span>email – send alert via email</span>

    -   <span>to\[string\] – recipient email address</span>

    -   <span>subject\[string\] – subject field</span>

-   <span>mongo – store alert to MongoDB</span>

    -   <span>host (optional)\[string\] – MongoDB instance host (defaults to *localhost*</span>

    -   <span>port (optional)\[number\] – MongoDB instance port (defaults to *27017*</span>

    -   <span>db\[string\] – database name</span>

    -   <span>collection\[string\] – collection name</span>

    -   <span>username (optional)\[string\] – MongoDB username (needed in case of enabled authentication)</span>

    -   <span>password (optional)\[string\] – MongoDB user’s password (needed in case of enabled authentication)</span>

-   <span>mark – add/modify value in alert</span>

    -   <span>path\[JSONPath\] – JSONPath in alert, if non-existent it is created</span>

    -   <span>value\[string\] – value to add</span>

-   <span>trap – send the message via TRAP IFC if it is opened for the reporter, otherwise the action takes no effect</span>

### Address Groups

Condition might refer to a named list of addresses. This feature extends Mentat Filtering. Each address group is identified by its string ID. Each address group must specify a file or a list of IP addresses/subnetworks. An address group file is a list of IP addresses/subnetworks separated by newline.

Implementation
==============

The new report2idea takes `config` argument which specifies path to a configuration file. This value is passed to an instance of Config class. The file is loaded and parsed by PyYAML module to dictionary. Afterwards Mentat Filtering is initialized in order to parse rules conditions. Next address groups are parsed. Depending on given configuration it can load and parse file address group which translates it to a list.

With address groups parsed it continues to parse custom actions. Each action is recognized by its type and an Action class is instantiated accordingly. This enables to create unlimited number of types and its actions in the future. Each action takes different arguments as specified in and implements `run` method which performs desired action. Right after custom actions parsing the implicit drop action is added as well.

The last part on initialization procedure is rules parsing. Each rule takes its dictionary representation, list of actions, list of address groups and optionally a Mentat Filtering instance (if not given each rule creates its own instance). Rule then parses its condition – finds all occurrences of address groups IDs and replaces them with a list of IP addresses/subnetworks and then parses the condition via Mentat Filtering. Finally, the rule links all its specified actions and elseactions with actions available in custom actions and in implicit actions.

Afterwards the report2idea `Run()` function starts to receive alerts from TRAP interface. Each received alert is converted to IDEA message, matched against available rules and the list of actions or elseactions is performed accordingly. This continues until TRAP interface or the reporter is terminated.

[1] http://nemea.liberouter.org/trap-ifcspec/

[2] https://pypi.python.org/pypi/pynspect/0.3
