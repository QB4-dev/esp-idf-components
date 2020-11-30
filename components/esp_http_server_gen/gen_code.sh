#!/bin/bash

SCRIPT_DIR=$(dirname $0)

[ ! -z "$1" ] && SERVER_DATA_DIR="$1" || echo "SERVER_DATA_DIR not found!"
[ ! -z "$2" ] && GENERATED_DATA_DIR="$2" || echo "GENERATED_DATA_DIR not found!"

if [ -z "$SERVER_DATA_DIR" ] || [ -z "$GENERATED_DATA_DIR" ]; then
	echo "usage: $0 SERVER_DATA_DIR GENERATED_DATA_DIR"
	exit 1
fi

mkdir -p $GENERATED_DATA_DIR

DATA_SOURCE=$GENERATED_DATA_DIR/httpd_data.c
DATA_HEADER=$GENERATED_DATA_DIR/httpd_data.h
HNDLR_SOURCE=$GENERATED_DATA_DIR/generated_handlers.c
	
# initialize data source file
echo "#include \"httpd_data.h\""     >  $DATA_SOURCE
echo ""                              >> $DATA_SOURCE	

# initialize data header file
echo "#pragma once"                           >  $DATA_HEADER
echo "#include <esp_http_server_gen.h>"       >> $DATA_HEADER
echo ""                                       >> $DATA_HEADER

# initialize handlers source file
echo "#include <esp_http_server.h>"  >  $HNDLR_SOURCE
echo "#include \"httpd_data.h\""     >> $HNDLR_SOURCE
echo ""                              >> $HNDLR_SOURCE

echo "const httpd_uri_t generated_handlers[] = {" >> $HNDLR_SOURCE
echo "	{.uri = \"/\", .method = HTTP_GET, .handler = esp_httpd_send_content, .user_ctx = &content_index_html}," >> $HNDLR_SOURCE

cd $SCRIPT_DIR
SCRIPT_BASE=$(pwd)
		
for file in $(find $SERVER_DATA_DIR -regextype posix-extended -regex '.*\.(html|css|js|svg|png|jpg|jpeg)'); do
	url=$(echo /${file#$SERVER_DATA_DIR/})
	file=$(basename ${file})
	ext="."${file##*.}
	vname=${file//[^[:alnum:]]/_}
	blob_start=$(echo _binary_${vname}_start)
	blob_size=$(echo  _binary_${vname}_size)
	
	#echo "$file  -> $blob_start size: $blob_size type: $ext url: $url"
	
	content_type="text/plain" #default
	case $ext in
		".html") content_type="text/html";;
		".css")  content_type="text/css";;
		".js")   content_type="text/javascript";;
		".svg")  content_type="image/svg+xml";;
		".png")  content_type="image/png";;
		".jpg")  content_type="image/jpeg";;
		".jpeg") content_type="image/jpeg";;
	  	*) echo "WARNING: $file - MIME type not recognized";;
	esac
	
	echo "extern unsigned char $blob_start[];" >> $DATA_SOURCE	
	echo "extern unsigned char $blob_size[];"  >> $DATA_SOURCE
	echo "esp_httpd_content_t content_$vname = {.type=\"$content_type\", .data=$blob_start, .len=(size_t)&$blob_size};" >> $DATA_SOURCE	
	echo ""      >> $DATA_SOURCE	
	
	echo "extern esp_httpd_content_t content_$vname;" >> $DATA_HEADER
	
	echo "	{.uri = \"$url\", .method = HTTP_GET, .handler = esp_httpd_send_content, .user_ctx = &content_$vname}," >> $HNDLR_SOURCE
done

echo "};" >> $HNDLR_SOURCE
echo ""   >> $HNDLR_SOURCE
echo "const size_t generated_handlers_num = sizeof(generated_handlers)/sizeof(generated_handlers[0]);"   >> $HNDLR_SOURCE

echo "$FILE_LIST"
exit 0
