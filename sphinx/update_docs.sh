#!/bin/sh

# extract the first docstring from a python file until the first "NOTE:"
# expects a python file as an argument
#   $1 = python file

function get_docstring() {
    docstring=$(awk '/^ *"""/ { if (++count == 2) exit; p = !p; next } /NOTE:/ { exit } p { print }' "$1")
}


# expects a string, a variable name, and an optional underline character
# $1 = string
# $2 = variable name
# $3 = underline character (optional)

function make_header() {
    if [ -z "$1" ]; then
        echo "Error: No text provided"
        return 1
    fi

    if [[ ! $2 =~ ^[a-zA-Z_][a-zA-Z0-9_]*$ ]]; then
        echo "Error: Invalid variable name"
        return 1
    fi

    if [ ! -z "$3" ] && [ ${#3} -ne 1 ]; then
        echo "Error: Underline character must be a single character"
        return 1
    fi

    local TEXT="$1"
    local LENGTH=${#TEXT}
    local UNDERLINE_CHAR=${3:-"="}
    local UNDERLINE=""
    for ((i=1; i<=LENGTH; i++)); do
        UNDERLINE+="${UNDERLINE_CHAR}"
    done
    local HEAD="${TEXT}\n${UNDERLINE}"
    eval $2="'$HEAD'"
}


function update_methods() {

    grep "^///" ../src/gc9a01.c | sed -e 's/^\/\/\/ //g' > _temp.md

    # Read _temp.md line by line. If the line starts "####", then it's a method.
    # when a method is found create a new file with the method name and the ".md" extension.
    # Then write the line to the new file. Continue writing lines to the new file until
    # the next method is found. Then close the file and continue reading _temp.md.
    # When the end of _temp.md is reached, delete _temp.md and exit.
    #!/bin/bash

    rm -f gc9a01.rst

    output_file="gc9a01.rst"
    echo "====================" > "$output_file"
    echo "GC9A01 Class Methods" >> "$output_file"
    echo "====================" >> "$output_file"
    echo "" >> "$output_file"

    while IFS= read -r line
    do
        if [[ $line == "### "* ]]; then
            name="${line:4}"
            name="${name%%(*}"
            echo "" >> "$output_file"
            line="${name}(${line#*\(}"
            line="\n.. class:: $line\n"
        fi

        if [[ $line == "#### "* ]]; then
            name="${line:6}"
            name="${name%%(*}"
            echo "" >> "$output_file"
            line="${name}(${line#*\(}"
            line="\n.. method:: $line\n"
        fi

        if [[ $line == "##### "* ]]; then
            line="\n- ${line:6}\n"
        fi

        if [[ $line == "///"* ]]; then
            line="${line:4}"
        fi

        echo -e "$line" >> "$output_file"

    done < "_temp.md"
    rm _temp.md
}


function update_configs() {
    rm -f configs/*.rst
    find ../tft_configs/ -mindepth 1 -type d | while IFS= read -r directory; do
        if [ "${directory}" != "../tft_configs" ]; then
            EXAMPLE=$(basename "${directory}")
            RST_FILE="configs/${EXAMPLE%.py}_config.rst"
            make_header "${EXAMPLE}" "MAIN_HEADER" "="
            {
                echo ".. _${directory#../}:"
                echo ""
                echo -e "${MAIN_HEADER}"

            } > "${RST_FILE}"

            for cfg_file in "${directory}"/tft_config*.py; do
                if [ -f "${cfg_file}" ]; then
                    get_docstring "${cfg_file}"
                    make_header "${cfg_file#../}" "SUB_HEADER" "^"
                    {
                        echo "${docstring}"
                        echo ""
                        echo -e "${SUB_HEADER}"
                        echo ""
                        echo ".. literalinclude:: ../${cfg_file}"
                        echo "   :language: python"
                        echo "   :linenos:"
                        echo "   :lines: 1-"
                        echo ""
                    } >> "${RST_FILE}"
                fi
            done
            if [ -f "${directory}/tft_buttons.py" ]; then
                get_docstring "${directory}/tft_buttons.py"
                make_header "${directory#../}/tft_buttons.py" "NAME" "^"
                {
                    echo ""
                    echo -e "${NAME}"
                    echo ""
                    echo "${docstring}"
                    echo ""
                    echo ".. literalinclude:: ../../tft_configs/${EXAMPLE}/tft_buttons.py"
                    echo "   :language: python"
                    echo "   :linenos:"
                    echo "   :lines: 1-"
                    echo ""
                } >> "${RST_FILE}"
            fi
        fi
    done
}


EXAMPLES=$(cat << 'EOF'
arcs.py
bitarray.py
feathers.py
fonts.py
hello.py
hello_world.py
hershey.py
jpg/alien.py
jpg/jpg_test.py
lines.py
mono_fonts/mono_fonts.py
noto_fonts/noto_fonts.py
pbitmap/bluemarble.py
pinball.py
prop_fonts/chango.py
proverbs/proverbs.py
roids.py
rotations.py
scroll.py
tiny_toasters/tiny_toasters.py
toasters/toasters.py
watch/watch.py
EOF
)

function update_examples() {
    rm -f examples/*.rst

    for file_name in $EXAMPLES; do
        EXAMPLE=$(basename "${file_name}")
        RST_FILE="examples/${EXAMPLE%.py}.rst"
        get_docstring "../examples/${file_name}"
        {
            echo ".. _${EXAMPLE%.py}:"
            echo ""
            echo "${docstring}"
            echo ""
            echo ".. literalinclude:: ../../examples/${file_name}"
            echo "   :language: python"
            echo "   :linenos:"
            echo "   :lines: 1-"
            echo ""
        } > "${RST_FILE}"
    done
}


UTILITES=$(cat << 'EOF'
create_png_examples.py
image_converter.py
sprites_converter.py
text_font_converter.py
write_font_converter.py
EOF
)

function update_utils() {
    rm -f utils/*.rst

    for file_name in $UTILITES; do
        EXAMPLE=$(basename "${file_name}")
        RST_FILE="utils/${EXAMPLE%.py}.rst"
        get_docstring "../utils/${file_name}"
        make_header "${EXAMPLE}" "NAME" "-"
        {
            echo ".. _${EXAMPLE%.py}:"
            echo ""
            echo -e "${NAME}"
            echo "${docstring}"
            echo ""
        } > "${RST_FILE}"
    done
}


function hack_index() {
    {
        echo "Index"
        echo "#####"
        echo ""
    } > genindex.rst
}


make clean
update_methods
update_configs
update_examples
update_utils
hack_index
make html
/usr/bin/rsync --progress --delete --progress -avz _build/html/* ../docs/ > /dev/null
touch ../docs/.nojekyll
make clean
