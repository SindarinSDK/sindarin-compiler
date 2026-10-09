__sn__PublicFields *make_public_fields(void)
{
    __sn__PublicFields *value = __sn__PublicFields__new();
    value->count = 4;
    value->text = strdup("seed");
    value->pointer = NULL;
    return value;
}
