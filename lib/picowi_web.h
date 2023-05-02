// PicoWi Web definitions, see http://iosoft.blog/picowi for details
//
// Copyright (c) 2022, Jeremy P Bentham
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define MAX_WEB_HANDLERS 5

#define HTTP_200_OK         "HTTP/1.1 200 OK\r\n"
#define HTTP_404_FAIL       "HTTP/1.1 404 Not Found\r\n"
#define HTTP_SERVER         "Server: picowi\r\n"
#define HTTP_NOCACHE        "Cache-Control: no-cache, no-store, must-revalidate\r\n"
#define HTTP_CONTENT_HTML   "Content-Type: text/html; charset=ISO-8859-1\r\n"
#define HTTP_CONTENT_JPEG   "Content-Type: image/jpeg\r\n"
#define HTTP_CONTENT_TEXT   "Content-Type: text/plain\r\n"
#define HTTP_CONTENT_BINARY "Content-Type: application/octet-stream\r\n"
#define HTTP_CONTENT_LENGTH "Content-Length: %d\r\n"
#define HTTP_ORIGIN_ANY     "Access-Control-Allow-Origin: *\r\n"
#define HTTP_TRANSFER_CHUNKED "Transfer-Encoding: chunked\r\n"
#define HTTP_MULTIPART      "Content-Type: multipart/x-mixed-replace; boundary=mjpeg_boundary\r\n"
#define HTTP_BOUNDARY       "\r\n--mjpeg_boundary\r\n"
#define HTTP_CONNECTION_CLOSE "Connection: close\r\n"
#define HTTP_HEADER_END     "\r\n"

typedef struct {
    char *req;
    web_handler_t handler;
} WEB_HANDLER;

int web_page_handler(char *req, web_handler_t handler);
int web_page_rx(int sock, char *req, int len);
int web_resp_add_data(int sock, BYTE *data, int dlen);
int web_resp_add_str(int sock, char *str);
int web_resp_add_content_len(int sock, int n);
int web_resp_send(int sock);

// EOF
