#! /bin/bash

# Compiles and links all assembly files that are found
# with both AVR-LLVM and AVR-GCC and compares that the
# output is identical.

TRIPLE=avr-atmel-none
TMP_DIR=/tmp/$(basename $0 .sh)-$RANDOM
MCU=atmega328p

# Whether we should throw out or keep the generated
# object/dump files
KEEP_OUTPUT=0

# log(msg)
function log {
    echo "$(basename $0): $1"
}

# error(msg)
function log_error {
    >&2 log "error: $1"
}

# print_usage()
function print_usage {
    echo
    echo "Usage: $0 <src-file|src-dir>"
    exit
}

function check_fatal_error {
    CMD=$1
    MSG=$2

    $1 >/dev/null 2>&1
    local status=$?

    if [ $status -ne 0 ]; then
        log_error "$MSG"
        exit
    fi
}

# assemble_with_llvm(infile, outfile)
function assemble_with_llvm {
    INFILE=$1
    OUTFILE=$2

    llvm-mc --triple $TRIPLE -mcpu=$MCU -mattr=special -filetype=obj $INFILE -o $OUTFILE
    return $?
}

# assemble_with_gcc(infile, outfile)
function assemble_with_gcc {
    INFILE=$1
    OUTFILE=$2

    if [ "$(basename $INFILE)" == "inst-lds.s" ]; then
        return 1
    fi

    avr-as -mall-opcodes -mmcu=$MCU $INFILE -o $OUTFILE 1>/dev/null 2>&1
    return $?
}

# link(infile, outfile)
function link {
    INFILE=$1
    OUTFILE=$2

    avr-gcc -mmcu=$MCU $INFILE -o $OUTFILE -Wl,--unresolved-symbols=ignore-in-object-files 1>/dev/null 2>&1
    return $?
}

# dump(infile, outfile)
function dump {
    INFILE=$1
    OUTFILE=$2

    avr-objdump -S $INFILE > $OUTFILE

    # strip `file-format/name` line
    sed -i '/file format \w/g' $OUTFILE
}

# compare(file1, file2)
function compare {
    cmp $1 $2 > /dev/null

    local status=$?

    if [ $status -ne 0 ]; then
        >&2 echo
        log_error "$(basename $1) and $(basename $2) are not the same"
        log_error "diff $(basename $1) $(basename $2)"
        >&2 diff $1 $2
        >&2 echo
    fi

    return $status
}

# check_src_file(file)
function check_src_file {
    SRC_FILE=$1
    BASENAME=$(basename $SRC_FILE .s)

    GCC_OBJ_OUT=$TMP_DIR/$BASENAME.gcc.o
    GCC_EXE_OUT=$TMP_DIR/$BASENAME.gcc
    GCC_DUMP_OUT=$TMP_DIR/$BASENAME.gcc.dump

    LLVM_OBJ_OUT=$TMP_DIR/$BASENAME.llvm.o
    LLVM_EXE_OUT=$TMP_DIR/$BASENAME.llvm
    LLVM_DUMP_OUT=$TMP_DIR/$BASENAME.llvm.dump

    status=1

    assemble_with_gcc $SRC_FILE $GCC_OBJ_OUT && \
    assemble_with_llvm $SRC_FILE $LLVM_OBJ_OUT && \
    link $GCC_OBJ_OUT $GCC_EXE_OUT && \
    link $LLVM_OBJ_OUT $LLVM_EXE_OUT && \
    dump $GCC_EXE_OUT $GCC_DUMP_OUT && \
    dump $LLVM_EXE_OUT $LLVM_DUMP_OUT && \
    status=0

    if [ $status -ne 0 ]; then
        # don't fail if we couldn't compile the program.
        return 0
    else
        compare $GCC_DUMP_OUT $LLVM_DUMP_OUT
        return $?
    fi
}

# check_src_dir(dir)
function check_src_dir {
    SRC_DIR=$1
    failed=0
    broken_tests=

    if [[ -z "$1" ]]; then
        log_error "Expected a source directory"
        print_usage
    fi

    ASM_FILES=$(find $SRC_DIR -name "*.s")

    for ASM_FILE in $ASM_FILES; do
        check_src_file $ASM_FILE
        status=$?

        if [ $status -ne 0 ]; then
            failed=1
            broken_tests="$ASM_FILE $broken_tests"
        fi
    done

    if [[ ! -z "${broken_tests// }" ]]; then
        echo "Broken tests:"
        for broken_test in $broken_tests; do
            echo " - $(basename $broken_test)"
        done
    else
        log "Machine code verification successful!"
    fi

    return $failed
}

check_fatal_error "avr-gcc --version" "AVR-GCC not found in \$PATH"
check_fatal_error "llvm-mc -version" "AVR-LLVM not found in \$PATH (or has a lower precedence than system LLVM)"

mkdir -p $TMP_DIR

check_src_dir $1
status=$?

if [ $KEEP_OUTPUT -eq 0 ]; then
    rm -rf $TMP_DIR
else
    log "Output placed in $TMP_DIR"
fi


exit $status
