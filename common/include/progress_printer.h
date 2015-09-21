/**
 * \file progress_printer.h
 * \brief Macros for printing a given character (dot by default) every N records -- header file.
 * \author Katerina Pilatova <xpilat05@stud.fit.vutbr.cz>
 * \date 2014
 */

#ifndef PROGRESS_PRINTER_H
#define PROGRESS_PRINTER_H

/**
 * Initial value of record counter.
 */

#define NMCM_CNT_VALUE 1

/**
 * \brief Declares progress structure prototype.
 * Prototype contains counter, limit and printed character.
 */

#define NMCM_PROGRESS_DECL struct nmcm_progress_t { \
                         int cnt, limit; \
                         char print_char; \
                      }; \

/**
 * \brief Declares structure trap_progress, pointer to this structure.
 * Uses pointer for better accessibility from functions other than main.
 * Initializes its limit value to 0.
 */

#define NMCM_PROGRESS_DEF struct nmcm_progress_t nmcm_progress; \
                          struct nmcm_progress_t *nmcm_progress_ptr = &nmcm_progress;\
                          nmcm_progress_ptr->print_char = 0; \
                          nmcm_progress_ptr->limit = 0;

/**
 * \brief Initializes progress variables.
 * Initializes limit value. If not inserted, prints error message and
 * carries out optional error command (e.g. returns nonzero value).
 * Initializes counter and progress character to default value.
 * \param[in]        a  Parsed progress limit.
 * \param[in]  err_cmd  Which error command is to be executed.
 */

#define NMCM_PROGRESS_INIT(a,err_cmd) do { \
              if (a <= 0) { \
                 fprintf(stderr,"Error: 'progress' argument has to be greater than zero.\n"); \
                 err_cmd; \
              } else { \
                 nmcm_progress_ptr->cnt = NMCM_CNT_VALUE; \
                 nmcm_progress_ptr->limit = (a); \
                 nmcm_progress_ptr->print_char = '.'; \
              } \
           } while (0);

/**
 * \brief Print a character ('.' by default) on every N-th invocation.
 */

#define NMCM_PROGRESS_PRINT do { \
              if (nmcm_progress_ptr->cnt == nmcm_progress.limit) { \
                 putchar(nmcm_progress_ptr->print_char); \
                 fflush(stdout); \
                 nmcm_progress_ptr->cnt = NMCM_CNT_VALUE; \
              } else { \
                 nmcm_progress_ptr->cnt++; \
              } \
           } while (0);

/**
 * \brief Print newline if progress printing is enabled.
 * Typically used before something else should be printed out or before
 * module termination.
 */

#define NMCM_PROGRESS_NEWLINE do { \
              if (nmcm_progress_ptr->limit > 0) { \
                 putchar('\n'); \
              } \
           } while (0);

#endif

