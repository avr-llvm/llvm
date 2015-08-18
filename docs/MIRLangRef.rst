========================================
Machine IR (MIR) Format Reference Manual
========================================

.. contents::
   :local:

.. warning::
  This is a work in progress.

Introduction
============

This document is a reference manual for the Machine IR (MIR) serialization
format. MIR is a human readable serialization format that is used to represent
LLVM's :ref:`machine specific intermediate representation
<machine code representation>`.

The MIR serialization format is designed to be used for testing the code
generation passes in LLVM.

Overview
========

The MIR serialization format uses a YAML container. YAML is a standard
data serialization language, and the full YAML language spec can be read at
`yaml.org
<http://www.yaml.org/spec/1.2/spec.html#Introduction>`_.

A MIR file is split up into a series of `YAML documents`_. The first document
can contain an optional embedded LLVM IR module, and the rest of the documents
contain the serialized machine functions.

.. _YAML documents: http://www.yaml.org/spec/1.2/spec.html#id2800132

High Level Structure
====================

Embedded Module
---------------

When the first YAML document contains a `YAML block literal string`_, the MIR
parser will treat this string as an LLVM assembly language string that
represents an embedded LLVM IR module.
Here is an example of a YAML document that contains an LLVM module:

.. code-block:: llvm

     --- |
       define i32 @inc(i32* %x) {
       entry:
         %0 = load i32, i32* %x
         %1 = add i32 %0, 1
         store i32 %1, i32* %x
         ret i32 %1
       }
     ...

.. _YAML block literal string: http://www.yaml.org/spec/1.2/spec.html#id2795688

Machine Functions
-----------------

The remaining YAML documents contain the machine functions. This is an example
of such YAML document:

.. code-block:: llvm

     ---
     name:            inc
     tracksRegLiveness: true
     liveins:
       - { reg: '%rdi' }
     body: |
       bb.0.entry:
         liveins: %rdi

         %eax = MOV32rm %rdi, 1, _, 0, _
         %eax = INC32r killed %eax, implicit-def dead %eflags
         MOV32mr killed %rdi, 1, _, 0, _, %eax
         RETQ %eax
     ...

The document above consists of attributes that represent the various
properties and data structures in a machine function.

The attribute ``name`` is required, and its value should be identical to the
name of a function that this machine function is based on.

The attribute ``body`` is a `YAML block literal string`_. Its value represents
the function's machine basic blocks and their machine instructions.

Machine Instructions Format Reference
=====================================

The machine basic blocks and their instructions are represented using a custom,
human readable serialization language. This language is used in the
`YAML block literal string`_ that corresponds to the machine function's body.

A source string that uses this language contains a list of machine basic
blocks, which are described in the section below.

Machine Basic Blocks
--------------------

A machine basic block is defined in a single block definition source construct
that contains the block's ID.
The example below defines two blocks that have an ID of zero and one:

.. code-block:: llvm

    bb.0:
      <instructions>
    bb.1:
      <instructions>

A machine basic block can also have a name. It should be specified after the ID
in the block's definition:

.. code-block:: llvm

    bb.0.entry:       ; This block's name is "entry"
       <instructions>

The block's name should be identical to the name of the IR block that this
machine block is based on.

Block References
^^^^^^^^^^^^^^^^

The machine basic blocks are identified by their ID numbers. Individual
blocks are referenced using the following syntax:

.. code-block:: llvm

    %bb.<id>[.<name>]

Examples:

.. code-block:: llvm

    %bb.0
    %bb.1.then

Successors
^^^^^^^^^^

The machine basic block's successors have to be specified before any of the
instructions:

.. code-block:: llvm

    bb.0.entry:
      successors: %bb.1.then, %bb.2.else
      <instructions>
    bb.1.then:
      <instructions>
    bb.2.else:
      <instructions>

The branch weights can be specified in brackets after the successor blocks.
The example below defines a block that has two successors with branch weights
of 32 and 16:

.. code-block:: llvm

    bb.0.entry:
      successors: %bb.1.then(32), %bb.2.else(16)

Live In Registers
^^^^^^^^^^^^^^^^^

The machine basic block's live in registers have to be specified before any of
the instructions:

.. code-block:: llvm

    bb.0.entry:
      liveins: %edi, %esi

The list of live in registers and successors can be empty. The language also
allows multiple live in register and successor lists - they are combined into
one list by the parser.

Miscellaneous Attributes
^^^^^^^^^^^^^^^^^^^^^^^^

The attributes ``IsAddressTaken``, ``IsLandingPad`` and ``Alignment`` can be
specified in brackets after the block's definition:

.. code-block:: llvm

    bb.0.entry (address-taken):
      <instructions>
    bb.2.else (align 4):
      <instructions>
    bb.3(landing-pad, align 4):
      <instructions>

.. TODO: Describe the way the reference to an unnamed LLVM IR block can be
   preserved.


.. TODO: Describe the parsers default behaviour when optional YAML attributes
   are missing.
.. TODO: Describe the syntax of the machine instructions.
.. TODO: Describe the syntax of the immediate machine operands.
.. TODO: Describe the syntax of the register machine operands.
.. TODO: Describe the syntax of the virtual register operands and their YAML
   definitions.
.. TODO: Describe the syntax of the register operand flags and the subregisters.
.. TODO: Describe the machine function's YAML flag attributes.
.. TODO: Describe the syntax for the global value, external symbol and register
   mask machine operands.
.. TODO: Describe the frame information YAML mapping.
.. TODO: Describe the syntax of the stack object machine operands and their
   YAML definitions.
.. TODO: Describe the syntax of the constant pool machine operands and their
   YAML definitions.
.. TODO: Describe the syntax of the jump table machine operands and their
   YAML definitions.
.. TODO: Describe the syntax of the block address machine operands.
.. TODO: Describe the syntax of the CFI index machine operands.
.. TODO: Describe the syntax of the metadata machine operands, and the
   instructions debug location attribute.
.. TODO: Describe the syntax of the target index machine operands.
.. TODO: Describe the syntax of the register live out machine operands.
.. TODO: Describe the syntax of the machine memory operands.
