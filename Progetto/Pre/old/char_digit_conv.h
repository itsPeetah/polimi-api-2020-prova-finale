int Char2Int(char c);
char Int2Char(int n);

int Char2Int(char c){
    return c - '0';
}
char Int2Char(int n){
    return '0' + n;
}