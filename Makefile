srcdir = src/
bindir = bin/
savedir = save/
docdir = doc/
SRC = $(wildcard $(srcdir)*.c)
HEAD = $(wildcard $(srcdir)*.h)
OBJ = $(subst $(srcdir),$(bindir),$(SRC:.c=.o))
RM = rm -ri $(bindir)*.o
CC = gcc -pthread -Wall -Wextra
PROG = executable
CP = cp $(srcdir)*.c $(savedir) ; cp $(srcdir)*.h $(savedir)


all : $(bindir)$(PROG) save

$(bindir)$(PROG): $(OBJ)
#on ne refait l'édition des liens que si un fichier objet à été recompilé
	$(CC) -o $@ $^ -lm

$(bindir)%.o : $(srcdir)%.c
#on ne recompile chaque fichier objet que si il est plus récent que l'ancien
	$(CC) -o $@ -c $< -lm


.PHONY:clean save

restore:
#on régénere les fichiers sources et les entêtes sauvegardés dans save dans le dossier src
	cp $(savedir)*.c $(srcdir)
	cp $(savedir)*.h $(srcdir)

give:
#décompresse une archive nécessaire à la génération du projet
	tar zxvf brillant_corentin.tar.gz

save : 
#on sauvegarde les fichiers sources du projet
	$(CP)

clean:
# on supprime tous les fichiers objets si on le désire
	$(RM)
