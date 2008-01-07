#!/bin/bash

cat > sql.qrc <<EOF
<!DOCTYPE RCC><RCC version="1.0">
<qresource>
EOF
find . -name \*.sql -exec echo "    <file>{}</file>" \; >> sql.qrc
cat >> sql.qrc <<EOF
</qresource>
</RCC>
EOF
