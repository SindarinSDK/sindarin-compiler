void mutate_pair(char *first, char *second) {
    *first = 'B';
    *second = first == second ? 'C' : 'D';
}
