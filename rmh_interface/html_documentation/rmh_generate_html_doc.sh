#!/bin/sh

# This script will use the gcc preprocessor to generate basic documentation based on the main RMH header file 'rmh_api.h'.
# It's not perfect as there are issues with unnecessary quotes and we can't easily link tags but it's the best I can do without
# making an entire application. Making this an application wouldn't be so bad but it would also require recompiling librmh for
# x86 so it could be linked in. For now this should be good enough...

SOURCE_DIR="${1}"
VERSION="${2}"
BRANCH="${3}"
DATE="$(date)"

cat << EOF
<!DOCTYPE html>
<html>
    <head>
        <style>
            body {
                font-family:"Arial";
                color: black;
                font-size:14px;
                text-align: left;
                background: White;
            }
            .version {
                font-size:12px;
                text-align: right;
                font-weight:bold;
                margin-bottom: 20px;
            }
            .title {
                font-family:"Impact";
                font-size: 32px;
                margin-bottom: 20px;
                text-align: center;
                font-weight:bold;
            }
            .tocheader {
                font-size: 24px;
                font-weight:bold;
            }
            .toc {
                font-family:"Impact";
                padding: 10px;
                color: black;
            }
            .toc ul {
                font-family:"Arial";
                margin: 0;
                margin-bottom: 100px;
                display: inline-block;
                background: #fff;
                margin-left: 35px;
                list-style-type: square;
                font-size: 18px;
            }
            .toc a {
                text-decoration: none;
            }
            .toc a:hover {
                color: #c40000;
                opacity: 0.7;
                -moz-opacity: 0.7;
                filter:alpha(opacity=70);
            }
            .api {
                border: 1px solid black;
                padding: 10px;
                background: Lavender;
                margin-bottom: 100px;
            }
            .apiTitle {
                font-size:28px;
                font-weight:bold;
                margin-bottom: 10px;
                text-align: left;
            }
            .apiType {
                font-size: 12px;
                font-weight:bold;
                text-align: right;
                float:right;
            }
            .apiSectionHeader {
                font-size:18px;
                font-weight:bold;
                display: inline-block;
                margin-top: 20px;
            }
            .apiSectionBody {
                margin-left: 20px;
                text-align: left;
            }
            .apiDeclaration {
                font-family:"Courier";
                margin-left: 20px;
                border: 1px solid black;
                padding: 10px;
                background: Gainsboro;
            }
            .apiParameters {
                font-size: 0px;
            }
            .apiParameter {
            }
            .apiParameterHeader {
                margin-left: 20px;
                font-size: 14px;
                font-weight:bold;
            }
            .apiParameterName {
                font-size: 14px;
                font-weight:bold;
                margin-left: 20px;
            }
            .apiParameterDirection {
                font-size: 12px;
                font-style: italic;
                margin-left: 20px;
            }
            .apiParameterDescription {
                margin-left: 20px;
                font-size: 14px;
            }
        </style>
    </head>

    <body>
        <a name="top"></a>
        <div class="version">
            ${DATE}<BR>${VERSION} [${BRANCH}]<BR>
        </div><BR>
        <div class="title">RDK MoCA HAL APIs (RMH)</div><BR>
EOF




###############################################################################################
cat << EOF
        <div id=toc_soc class="toc">
            <span class="tocheader">RMH Control APIs (SoC Implemented)</span><BR>
            <ul>
EOF
            # Generate HTML table of contents
            gcc -E -P "${SOURCE_DIR}/rmh_interface/html_documentation/rmh_generate_html_toc.h" \
                        -I"${SOURCE_DIR}/rmh_interface" \
                        -I"${SOURCE_DIR}/rmh_lib" | \
                                  sed -e 's/__POSTPROC_POUND__/#/g' | \
                                  sort
cat << EOF
            </ul>
        </div><BR>
EOF
###############################################################################################





###############################################################################################
cat << EOF
        <div id=toc_generic class="toc">
            <span class="tocheader">RMH Helper APIs</span><BR>
            <ul>
EOF
            # Generate HTML table of contents
            gcc -E -P -DGENERIC_ONLY "${SOURCE_DIR}/rmh_interface/html_documentation/rmh_generate_html_toc.h" \
                        -I"${SOURCE_DIR}/rmh_interface" \
                        -I"${SOURCE_DIR}/rmh_lib" | \
                                  sed -e 's/__POSTPROC_POUND__/#/g' | \
                                  sort

cat << EOF
            </ul>
        </div>
EOF
###############################################################################################





###############################################################################################

        # Generate HTML table of contents
        gcc -E -P "${SOURCE_DIR}/rmh_interface/html_documentation/rmh_generate_html_apis.h" -I"${SOURCE_DIR}/rmh_interface" -I"${SOURCE_DIR}/rmh_lib"

###############################################################################################


cat << EOF
    </body>
</html>"
EOF
             #
