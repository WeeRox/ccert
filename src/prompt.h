#ifndef __PROMPT_H
#define __PROMPT_H

char *prompt_text(char *prompt);
int prompt_yesno(char *prompt, int def);
int prompt_select(char *prompt, char **choices, int *retval, int n);
void prompt_enter(char *prompt);

#endif /* __PROMPT_H */
