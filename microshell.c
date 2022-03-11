/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   microshell.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: tkhabous <marvin@42.fr>                    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/03/11 01:12:07 by tkhabous          #+#    #+#             */
/*   Updated: 2022/03/11 01:12:20 by tkhabous         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

# include <unistd.h>
# include <stdlib.h>
# include <sys/wait.h>
# include <string.h>


/**//**************************/
/**/typedef struct s_tokens {   //
/**/    char **cmds;            //
/**/    int std_out;            //  A struct used to keep track of commands and standard input and output
/**/    int std_in;             //  it is also al linked list :)
/**/    struct s_tokens * next; //
/**/}   t_tokens;               //
/**//*************************/

int ft_strlen(char *str)
{
    int i = 0;
    while (str[i] != '\0')                              // no need to explain :/
        i++;
    return i;
}

void print_error(char *error, int n)                    // print an error and exit only when n == 1
{
    write(2, error, ft_strlen(error));
    if (n)
        exit(1);
}

t_tokens *init_token()                                  // allocate a new structure and initialize the values
{
    t_tokens *tmp;

    tmp = malloc(sizeof(t_tokens));
    if (tmp == NULL)
        print_error("Error: FATAL\n", 1);
    tmp->cmds = malloc(sizeof(char *) * 1024);
    if (tmp->cmds == NULL)
        print_error("Error: FATAL\n", 1);
    tmp->std_in = 0;     /* standard output */
    tmp->std_out = 1;    /* standard input  */
    return tmp;
}

int add_cmd(t_tokens *t_token, char **argv, int i)      // add commands to the struct variables
{
    int j = 0;
    while (argv[i] && strcmp(argv[i], ";")              // unless you reach a ; or a | keep adding commands
            && strcmp(argv[i], "|"))
    {
        t_token->cmds[j] = argv[i];
        i++;
        j++;
    }
    i++;
    t_token->cmds[j] = NULL;                            // set the last command to NULL this is important because exceve need the commands to be NULL terminated see line 94
    return i;
}

void ft_pipe(t_tokens *t_token)                         // create a pipe and set the file descriptors to the structure variables
{
    int fds[2];

    pipe(fds);
    t_token->next = init_token();                       //                                 ____
    t_token->next->std_in = fds[0];                     // go see how a pipe work :')  Fd0 ____ Fd1   https://www.youtube.com/watch?v=6u_iPGVkfZ4&t=478s
    t_token->std_out = fds[1];
}

void exec_cmd(t_tokens * t_token, int *start, int i, char **env)
{
    if (!(strcmp(t_token->cmds[0], "cd")))              // if cmd == cd
    {
        if (t_token->cmds[2] && !t_token->cmds[1])      // special case : cd "" hello
            print_error("Error cd: Bad arguments\n", 0);
        else if (chdir(t_token->cmds[1]) != 0)          // chdir is an aliase for cd upon success return 0
            print_error("Error cd: cannot\
                            change directory\n", 0);
        *start = i;                                     // skip non useful commands
    }
    // if any command other than cd just use execve.
    int pid = fork();                                   // create a child to handle execve
    if (pid < 0)
        print_error("Error: FATAL\n", 1);               // a non successfull fork could be the cause of full memory
    if (pid == 0)                                       // the child has taken over
    {
        if (t_token->std_out != 1)                      // this means that a pipe has been created because the default value has been changed by ft_pipe()
        {
            close(t_token->next->std_in);
            if (dup2(t_token->std_out, 1) == -1)
                print_error("Error : FATAL\n", 1);      // duplicate file discreptor fds[1] with 1 which is the standard output
        }
        if (t_token->std_in != 0)                       // this means that a pipe has been created because the default value has been changed by ft_pipe()
        {
            if (dup2(t_token->std_in, 0) == -1)
                print_error("Error : FATAL\n", 1);      // duplicate file discreptor fds[0] with 0 which is the standard input
        }
        execve(t_token->cmds[0], t_token->cmds, env);   // execve will execute the command for as and exit the child upon error it will not exit this child
        print_error("Error : execve wouldn't work", 1); // if execve worked as it should, this line wouldn't have been executed
    }
    // both child and parent will execute below code
    if (t_token->std_out != 1)                          // if this condition is true then std_out points to fd[1] which we should close when we are done
        close(t_token->std_out);
    if (t_token->std_in != 0)                           // if this condition is true then std_in points to fd[0] which we should close when we are done
        close(t_token->std_in);
    if (t_token->std_out == 1)  
    {                           
        while (*start < i)      
        {                       
            waitpid(0, NULL, 0);                        // wait for the child to exit
            (*start)++;         
        }                       
    }
}

void ft_free(t_tokens *t_token)
{
    if (t_token)
    {
        if (t_token->cmds)
            free(t_token->cmds);
        free(t_token);
        t_token = NULL;                                 // make it NULL so that in line 132 it will be reinitialized
    }
}

int main(int argc, char **argv, char **env)             /*** Made with love, by @Taha kh. https://github.com/khabouss/exam-rank-04 ****/
{
    t_tokens *t_token;
    t_tokens *before;
    int i = 1;
    int start = i;
    while (i < argc)
    {
        if (t_token == NULL || start == i)              // initialize the structure     _
            t_token = init_token();                     //                               |
        if (!strcmp(argv[i], ";"))                      // if ; skip it                  |
        {                                               //                               |
            i++;                                        //                               |
            continue;                                   //                               |
        }                                               //                                \ Parsing
        i = add_cmd(t_token, argv, i);                  // add commands to cmds           / Part
        if (argv[i - 1] && !(strcmp(argv[i-1], "|")))   // check for pipes               |
        {                                               //                               |
            ft_pipe(t_token);                           // call pipe()                   -
        }                                           
        
        exec_cmd(t_token, &start, i, env);              // execution part                - Execution Part
        before = t_token;                               // copy t_token
        if (t_token->std_out != 1)                      // it means a pipe has been created before
            t_token = t_token->next;
        ft_free(before);                                // free the old structure memory
    }
    return (0);
}
