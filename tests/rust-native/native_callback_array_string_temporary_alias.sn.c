static char *last_element;
static void release_element(void *element) { sn_cleanup_str((char **)element); }
SnArray *produce(void) {
    SnArray *values=sn_array_new(sizeof(char *),1);
    values->elem_tag=SN_TAG_STRING;
    values->elem_release=release_element;
    values->elem_copy=sn_copy_str;
    last_element=strdup("a");
    sn_array_push(values,&last_element);
    return values;
}
bool same(const char *value) { return value==last_element; }
bool retained(void) { return last_element && strcmp(last_element,"a")==0; }
