static __sn__Record *remembered_value;
void remember(__sn__Record *value) { remembered_value = value; }
bool remembered(__sn__Record *value) { return value == remembered_value; }
bool same_text(__sn__Record *value, const char *text) { return value->__sn__label == text; }
void replace(__sn__Record *left, __sn__Record *right) {
    left->__sn__count += 7;
    right->__sn__count += left->__sn__count;
    free(left->__sn__label);
    left->__sn__label = strdup("native");
    free(right->__sn__leaf.__sn__name);
    right->__sn__leaf.__sn__name = strdup("native_leaf");
    right->__sn__leaf.__sn__mark = 'z';
}
void nil_text(__sn__Record *value) {
    free(value->__sn__label);
    value->__sn__label = NULL;
    free(value->__sn__leaf.__sn__name);
    value->__sn__leaf.__sn__name = NULL;
}
