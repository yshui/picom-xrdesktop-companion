// Generated by gir (https://github.com/gtk-rs/gir @ 2358cc24efd2)
// from ../../gir-files (@ eab91ba8f88b)
// from ../../xrd-gir-files (@ 2f1ef32fd867)
// DO NOT EDIT

#include "manual.h"
#include <stdio.h>

#define PRINT_CONSTANT(CONSTANT_NAME) \
    printf("%s;", #CONSTANT_NAME); \
    printf(_Generic((CONSTANT_NAME), \
                    char *: "%s", \
                    const char *: "%s", \
                    char: "%c", \
                    signed char: "%hhd", \
                    unsigned char: "%hhu", \
                    short int: "%hd", \
                    unsigned short int: "%hu", \
                    int: "%d", \
                    unsigned int: "%u", \
                    long: "%ld", \
                    unsigned long: "%lu", \
                    long long: "%lld", \
                    unsigned long long: "%llu", \
                    float: "%f", \
                    double: "%f", \
                    long double: "%ld"), \
           CONSTANT_NAME); \
    printf("\n");

int main() {
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_DOUBLE);
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_FLOAT);
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_INT16);
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_INT32);
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_INT8);
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_UINT16);
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_UINT32);
    PRINT_CONSTANT((gint) G3K_ATTRIBUTE_TYPE_UINT8);
    PRINT_CONSTANT((gint) G3K_CONTAINER_ATTACHMENT_HAND);
    PRINT_CONSTANT((gint) G3K_CONTAINER_ATTACHMENT_HEAD);
    PRINT_CONSTANT((gint) G3K_CONTAINER_ATTACHMENT_NONE);
    PRINT_CONSTANT((gint) G3K_CONTAINER_HORIZONTAL);
    PRINT_CONSTANT((gint) G3K_CONTAINER_NO_LAYOUT);
    PRINT_CONSTANT((gint) G3K_CONTAINER_RELATIVE);
    PRINT_CONSTANT((gint) G3K_CONTAINER_VERTICAL);
    PRINT_CONSTANT((gint) G3K_HOVER_MODE_BUTTONS);
    PRINT_CONSTANT((gint) G3K_HOVER_MODE_EVERYTHING);
    PRINT_CONSTANT((guint) G3K_INTERACTION_BUTTON);
    PRINT_CONSTANT((guint) G3K_INTERACTION_DESTROY_WITH_PARENT);
    PRINT_CONSTANT((guint) G3K_INTERACTION_DRAGGABLE);
    PRINT_CONSTANT((guint) G3K_INTERACTION_HOVERABLE);
    PRINT_CONSTANT((guint) G3K_INTERACTION_MANAGED);
    PRINT_CONSTANT((gint) G3K_LOADER_ERROR_FAIL);
    PRINT_CONSTANT((gint) G3K_RENDER_EVENT_FRAME_END);
    PRINT_CONSTANT((gint) G3K_RENDER_EVENT_FRAME_START);
    PRINT_CONSTANT((gint) XRD_TRANSFORM_LOCK_NONE);
    PRINT_CONSTANT((gint) XRD_TRANSFORM_LOCK_PUSH_PULL);
    PRINT_CONSTANT((gint) XRD_TRANSFORM_LOCK_SCALE);
    return 0;
}