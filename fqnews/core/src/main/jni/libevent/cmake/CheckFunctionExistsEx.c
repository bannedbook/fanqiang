#ifdef CHECK_FUNCTION_EXISTS

#ifndef _WIN32
char CHECK_FUNCTION_EXISTS();
#endif

#ifdef __CLASSIC_C__
int main(){
  int ac;
  char*av[];
#else
int main(int ac, char*av[]){
#endif
#ifdef _WIN32
  void * p = &CHECK_FUNCTION_EXISTS;
#else
  CHECK_FUNCTION_EXISTS();
#endif
  if(ac > 1000)
    {
    return *av[0];
    }
  return 0;
}

#else  /* CHECK_FUNCTION_EXISTS */

#  error "CHECK_FUNCTION_EXISTS has to specify the function"

#endif /* CHECK_FUNCTION_EXISTS */
