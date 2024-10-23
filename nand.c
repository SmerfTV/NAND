#include "nand.h"
#include <stdbool.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <stdint.h>

/*
Program jest dynamicznie ładowaną biblioteką napisaną w języku C, która umożliwia
tworzenie i obsługę kombinacyjnych układów boolowskich złożonych z bramek NAND.
Biblioteka pozwala na tworzenie bramek NAND o dowolnej liczbie wejść (n >= 0)
oraz jednym wyjściu.
 */

//Stala globalna ustalajaca startowa liczbe outputow
unsigned default_output_size = 5;

// Struktura reprezentująca bramkę NAND
typedef struct nand {
    unsigned input_size;
    unsigned output_size;
    struct nand **inputs;
    struct nand **outputs;
    unsigned connected_outputs;
    bool is_signal;
    bool const *signal;
    bool visited;
    bool cycleChecked;
    ssize_t value;
    bool gate_signal;
} nand_t;

// Struktura przechowująca parę wartości
typedef struct pair {
    bool result;
    ssize_t path;
} pair;

// Funkcja tworząca nową bramkę NAND
nand_t* nand_new(unsigned n) {
    nand_t *new_gate = (nand_t *) malloc(sizeof(nand_t));
    if (new_gate == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    new_gate->input_size = n;
    new_gate->outputs = NULL;
    new_gate->inputs = NULL;
    new_gate->connected_outputs = 0;
    new_gate->is_signal = false;
    new_gate->signal = NULL;
    new_gate->visited = false;
    new_gate->cycleChecked = false;
    new_gate->value = 0;
    new_gate->gate_signal = false;
    if (n > 0) {
        new_gate->inputs = (nand_t **) malloc(n * sizeof(nand_t*));
        if (new_gate->inputs == NULL) {
            errno = ENOMEM;
            free(new_gate);
            return NULL;
        }
        for (unsigned k = 0; k < n; k++) {
            new_gate->inputs[k] = NULL;
        }
    }

    return new_gate;
}

// Funkcja szukajaca bramki this w inputach bramki g i odlaczajaca ją wiele razy
// (uzywana do usuwania bramki this)
void nand_disconnect_from_input(nand_t *g, nand_t *this) {
    if (g == NULL || this == NULL) {
        return;
    }
    for (unsigned k = 0; k < g->input_size; k++) {
        if (g->inputs[k] == this) {
            g->inputs[k] = NULL;
        }
    }
}

//Funkcja szukająca bramki "this" w outputach bramki g i ja odlaczajaca (raz)
void nand_disconnect_from_output(nand_t *g, nand_t *this) {
    if (g == NULL || this == NULL) {
        return;
    }
    for (int k = 0; k < (int) g->output_size; k++) {

        if (g->outputs[k] == this) {
            g->outputs[k] = NULL;
            (g->connected_outputs)--;
            if (k != (int) g->connected_outputs) {
                g->outputs[k] = g->outputs[g->connected_outputs];
                k--;
                g->outputs[g->connected_outputs] = NULL;
            }
            if ((g->connected_outputs * 2) + 1 < g->output_size) {
                //Jezeli po odlaczeniu nie mam zadnego outputu- ustawiam tablice na NULL
                if(g->connected_outputs == 0) {
                free(g->outputs);
                g->outputs = NULL;
                }
                else {
                    //Jezeli rozmiar jest inny od domyslnego, realokuje na mniejszy
                    if(g->output_size != default_output_size) {
                        nand_t **new_outputs = realloc(
                            g->outputs,
                            sizeof(nand_t *) * ((g->output_size) / 2)
                        );
                        if (new_outputs == NULL) {
                            errno = ENOMEM;
                            return;
                        }
                        g->outputs = new_outputs;
                        g->output_size = g->output_size / 2;
                    }
                }
            }
            return;

        }
    }
}

// Funkcja usuwająca bramkę NAND
void nand_delete(nand_t *g) {
    if (g == NULL) {
        return;
    }
    if (g->inputs != NULL) {
        for (unsigned k = 0; k < g->input_size; k++) {
            nand_t *w = g->inputs[k];
            if (w != NULL) {
                if (w->is_signal == false) {
                    nand_disconnect_from_output(w, g);
                } else {
                    g->inputs[k] = NULL;
                    free(w);
                }
            }
        }
        free(g->inputs);
    }
    if (g->outputs != NULL) {
        for (unsigned k = 0; k < g->output_size; k++) {
            nand_t *w = g->outputs[k];
            if (w != NULL) {
                nand_disconnect_from_input(w, g);
            }
        }
        free(g->outputs);
    }
    free(g);
}

// Funkcja łącząca dwie bramki NAND
int nand_connect_nand(nand_t *g_out, nand_t *g_in, unsigned k) {
    if (g_out == NULL || g_in == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (k >= g_in->input_size) {
        errno = EINVAL;
        return -1;
    }
    if (g_in->inputs[k] != NULL) {
        if (g_in->inputs[k] == g_out) {
            return 0;
        }
    }
    if (g_out->outputs == NULL) {
        g_out->outputs = (nand_t **) malloc(default_output_size * sizeof(nand_t*));
        if (g_out->outputs == NULL) {
            errno = ENOMEM;
            return -1;
        }
        g_out->output_size = default_output_size;
        g_out->connected_outputs = 0;
        for (unsigned i = 0; i < default_output_size; i++) {
            g_out->outputs[i] = NULL;
        }
    }
    if (g_out->output_size == g_out->connected_outputs) {
        nand_t **new_outputs = realloc(
            g_out->outputs,
            2 * g_out->output_size * sizeof(nand_t*)
        );
        if (new_outputs == NULL) {
            errno = ENOMEM;
            return -1;
        }
        g_out->outputs = new_outputs;
        g_out->output_size = g_out->output_size * 2;
        for (unsigned i = g_out->connected_outputs; i < g_out->output_size; i++) {
            g_out->outputs[i] = NULL;
        }
    }
    g_out->outputs[g_out->connected_outputs] = g_in;
    (g_out->connected_outputs)++;
    if (g_in->inputs[k] != NULL) {
        nand_t* p = g_in->inputs[k];
        if (p->is_signal) {
            g_in->inputs[k] = NULL;
            free(p);
        } else {
            nand_disconnect_from_output(p, g_in);
            g_in->inputs[k] = NULL;
        }
    }
    g_in->inputs[k] = g_out;
    return 0;
}

// Funkcja podlaczajaca sygnał do bramki NAND
int nand_connect_signal(bool const *s, nand_t *g, unsigned k) {
    if (g == NULL || s == NULL) {
        errno = EINVAL;
        return -1;
    }
    if (k >= g->input_size) {
        errno = EINVAL;
        return -1;
    }
    nand_t* sig = malloc(sizeof(nand_t));
    if (sig == NULL) {
        errno = ENOMEM;
        return -1;
    }
    sig->input_size = 0;
    sig->output_size = 0;
    sig->inputs = NULL;
    sig->outputs = NULL;
    sig->connected_outputs = 0;
    sig->visited = false;
    sig->cycleChecked = false;
    sig->value = 0;
    sig->gate_signal = false;
    sig->is_signal = true;
    sig->signal = s;
    if (g->inputs[k] != NULL) {
        nand_t* p = g->inputs[k];
        if (p->is_signal) {
            g->inputs[k] = NULL;
            free(p);
        } else {
            nand_disconnect_from_output(p, g);
            g->inputs[k] = NULL;
        }
    }
    g->inputs[k] = sig;
    return 0;
}

// Funkcja sprawdzająca cykl w grafie bramek NAND
bool nand_check_for_cycle_and_null(nand_t *g) {
    if (g == NULL) return false;
    bool result = false;
    if (g->visited == false) {
        g->visited = true;
        for (unsigned i = 0; i < g->input_size; i++) {
            if (g->inputs[i] == NULL) result = true;
            if (g->inputs[i] != NULL) {
                bool local_result = nand_check_for_cycle_and_null(g->inputs[i]);
                if (local_result == true) result = true;
            }
        }
        g->cycleChecked = true;
    } else {
        if (g->cycleChecked == false) result = true;
    }
    return result;
}

// Funkcja czyszcząca po sprawdzeniu cyklu
void nand_clean_after_cycle_check(nand_t *g) {
    if (g == NULL) return;
    if (g->visited == true) {
        g->visited = false;
        g->cycleChecked = false;
        for (unsigned i = 0; i < g->input_size; i++) {
            if (g->inputs[i] != NULL) {
                nand_clean_after_cycle_check(g->inputs[i]);
            }
        }
    }
}

// Funkcja pojedynczej ewaluacji bramki NAND
pair nand_single_evaluate(nand_t *g) {
    pair evaluated_values = {false, 0};
    if (g == NULL) return evaluated_values;
    if (g->visited == true) {
        return (pair) {g->gate_signal, g->value};
    } else {
        ssize_t max = 0;
        if (g->is_signal == true) {
            g->visited = true;
            pair m = {false, 0};
            m.result = *g->signal;
            g->gate_signal = m.result;
            g->value = m.path;
            return m;
        }
        for (unsigned i = 0; i < g->input_size; i++) {
            if (g->inputs[i] != NULL) {
                nand_t *gate = g->inputs[i];
                pair ig_evaluated_values = {false, 0};
                if (gate->is_signal) {
                    ig_evaluated_values.path = 0;
                    ig_evaluated_values.result = *(gate->signal);
                } else {
                    ig_evaluated_values = nand_single_evaluate(gate);
                }
                if (ig_evaluated_values.result == false) evaluated_values.result = true;
                if (ig_evaluated_values.path > max) max = ig_evaluated_values.path;
            }
        }
        g->visited = true;
        if (g->input_size == 0) {
            g->value = 0;
            evaluated_values.path = 0;
        } else {
            evaluated_values.path = max + 1;
            g->value = max + 1;
            g->gate_signal = evaluated_values.result;
        }
    }
    return evaluated_values;
}

// Funkcja czyszcząca po ewaluacji bramek
void nand_clean_after_evaluate(nand_t *g) {
    if (g == NULL) return;
    if (g->visited == true) {
        g->visited = false;
        g->gate_signal = false;
        g->value = 0;
        for (unsigned i = 0; i < g->input_size; i++) {
            if (g->inputs[i] != NULL) {
                nand_clean_after_evaluate(g->inputs[i]);
            }
        }
    }
}

// Funkcja obliczajaca sciezke krytyczna tablicy bramek NAND
ssize_t nand_evaluate(nand_t **g, bool *s, size_t m) {
    if (g == NULL || s == NULL || m <= 0) {
        errno = EINVAL;
        return -1;
    }
    for (size_t i = 0; i < m; i++) {
        if (g[i] == NULL) {
            errno = EINVAL;
            return -1;
        }
    }
    for (size_t i = 0; i < m; i++) {
        bool check = nand_check_for_cycle_and_null(g[i]);
        if (check == true) {
            for (size_t k = 0; k <= i; k++) {
                nand_clean_after_cycle_check(g[k]);
            }
            errno = ECANCELED;
            return -1;
        } else {
            nand_clean_after_cycle_check(g[i]);
        }
    }
    ssize_t max = 0;
    for (size_t i = 0; i < m; i++) {
        pair res = nand_single_evaluate(g[i]);
        s[i] = res.result;
        if (res.path > max) max = res.path;
    }
    for (size_t i = 0; i < m; i++) {
        nand_clean_after_evaluate(g[i]);
    }
    return max;
}

// Funkcja zwracająca liczbę połączeń outputów
ssize_t nand_fan_out(nand_t const *g) {
    if (g == NULL) {
        errno = EINVAL;
        return -1;
    }
    ssize_t k = (ssize_t) g->connected_outputs;
    return k;
}

// Funkcja zwracająca wskaźnik na input
void* nand_input(nand_t const *g, unsigned k) {
    if (g == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (k >= g->input_size) {
        errno = EINVAL;
        return NULL;
    }
    if (g->inputs[k] == NULL) {
        errno = 0;
        return NULL;
    }
    nand_t* p = g->inputs[k];
    if (p->is_signal) {
        return (void*) p->signal;
    } else {
        return (void*) p;
    }
}

// Funkcja zwracająca wskaźnik na output
nand_t* nand_output(nand_t const *g, ssize_t k) {
    if (g == NULL) {
        errno = EINVAL;
        return NULL;
    }
    if (k < 0 || k >= g->connected_outputs) {
        errno = EINVAL;
        return NULL;
    }
    return g->outputs[k];
}
