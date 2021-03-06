#ifndef DIR_H
#define DIR_H

/* See Documentation/technical/api-directory-listing.txt */

#include "strbuf.h"

struct dir_entry {
	unsigned int len;
	char name[FLEX_ARRAY]; /* more */
};

#define EXC_FLAG_NODIR 1
#define EXC_FLAG_ENDSWITH 4
#define EXC_FLAG_MUSTBEDIR 8
#define EXC_FLAG_NEGATIVE 16

/*
 * Each .gitignore file will be parsed into patterns which are then
 * appended to the relevant exclude_list (either EXC_DIRS or
 * EXC_FILE).  exclude_lists are also used to represent the list of
 * --exclude values passed via CLI args (EXC_CMDL).
 */
struct exclude_list {
	int nr;
	int alloc;
	struct exclude {
		const char *pattern;
		int patternlen;
		int nowildcardlen;
		const char *base;
		int baselen;
		int flags;
	} **excludes;
};

/*
 * The contents of the per-directory exclude files are lazily read on
 * demand and then cached in memory, one per exclude_stack struct, in
 * order to avoid opening and parsing each one every time that
 * directory is traversed.
 */
struct exclude_stack {
	struct exclude_stack *prev; /* the struct exclude_stack for the parent directory */
	char *filebuf; /* remember pointer to per-directory exclude file contents so we can free() */
	int baselen;
	int exclude_ix;
};

struct dir_struct {
	int nr, alloc;
	int ignored_nr, ignored_alloc;
	enum {
		DIR_SHOW_IGNORED = 1<<0,
		DIR_SHOW_OTHER_DIRECTORIES = 1<<1,
		DIR_HIDE_EMPTY_DIRECTORIES = 1<<2,
		DIR_NO_GITLINKS = 1<<3,
		DIR_COLLECT_IGNORED = 1<<4
	} flags;
	struct dir_entry **entries;
	struct dir_entry **ignored;

	/* Exclude info */
	const char *exclude_per_dir;
	struct exclude_list exclude_list[3];
	/*
	 * We maintain three exclude pattern lists:
	 * EXC_CMDL lists patterns explicitly given on the command line.
	 * EXC_DIRS lists patterns obtained from per-directory ignore files.
	 * EXC_FILE lists patterns from fallback ignore files.
	 */
#define EXC_CMDL 0
#define EXC_DIRS 1
#define EXC_FILE 2

	/*
	 * Temporary variables which are used during loading of the
	 * per-directory exclude lists.
	 *
	 * exclude_stack points to the top of the exclude_stack, and
	 * basebuf contains the full path to the current
	 * (sub)directory in the traversal.
	 */
	struct exclude_stack *exclude_stack;
	char basebuf[PATH_MAX];
};

#define MATCHED_RECURSIVELY 1
#define MATCHED_FNMATCH 2
#define MATCHED_EXACTLY 3
extern char *common_prefix(const char **pathspec);
extern int match_pathspec(const char **pathspec, const char *name, int namelen, int prefix, char *seen);
extern int match_pathspec_depth(const struct pathspec *pathspec,
				const char *name, int namelen,
				int prefix, char *seen);
extern int within_depth(const char *name, int namelen, int depth, int max_depth);

extern int fill_directory(struct dir_struct *dir, const char **pathspec);
extern int read_directory(struct dir_struct *, const char *path, int len, const char **pathspec);

extern int is_excluded_from_list(const char *pathname, int pathlen, const char *basename,
				 int *dtype, struct exclude_list *el);
struct dir_entry *dir_add_ignored(struct dir_struct *dir, const char *pathname, int len);

/*
 * these implement the matching logic for dir.c:excluded_from_list and
 * attr.c:path_matches()
 */
extern int match_basename(const char *, int,
			  const char *, int, int, int);
extern int match_pathname(const char *, int,
			  const char *, int,
			  const char *, int, int, int);

/*
 * The is_excluded() API is meant for callers that check each level of leading
 * directory hierarchies with is_excluded() to avoid recursing into excluded
 * directories.  Callers that do not do so should use this API instead.
 */
struct path_exclude_check {
	struct dir_struct *dir;
	struct exclude *exclude;
	struct strbuf path;
};
extern void path_exclude_check_init(struct path_exclude_check *, struct dir_struct *);
extern void path_exclude_check_clear(struct path_exclude_check *);
extern struct exclude *last_exclude_matching_path(struct path_exclude_check *, const char *,
						  int namelen, int *dtype);
extern int is_path_excluded(struct path_exclude_check *, const char *, int namelen, int *dtype);


extern int add_excludes_from_file_to_list(const char *fname, const char *base, int baselen,
					  char **buf_p, struct exclude_list *el, int check_index);
extern void add_excludes_from_file(struct dir_struct *, const char *fname);
extern void parse_exclude_pattern(const char **string, int *patternlen, int *flags, int *nowildcardlen);
extern void add_exclude(const char *string, const char *base,
			int baselen, struct exclude_list *el);
extern void clear_exclude_list(struct exclude_list *el);
extern int file_exists(const char *);

extern int is_inside_dir(const char *dir);
extern int dir_inside_of(const char *subdir, const char *dir);

static inline int is_dot_or_dotdot(const char *name)
{
	return (name[0] == '.' &&
		(name[1] == '\0' ||
		 (name[1] == '.' && name[2] == '\0')));
}

extern int is_empty_dir(const char *dir);

extern void setup_standard_excludes(struct dir_struct *dir);

#define REMOVE_DIR_EMPTY_ONLY 01
#define REMOVE_DIR_KEEP_NESTED_GIT 02
#define REMOVE_DIR_KEEP_TOPLEVEL 04
extern int remove_dir_recursively(struct strbuf *path, int flag);

/* tries to remove the path with empty directories along it, ignores ENOENT */
extern int remove_path(const char *path);

extern int strcmp_icase(const char *a, const char *b);
extern int strncmp_icase(const char *a, const char *b, size_t count);
extern int fnmatch_icase(const char *pattern, const char *string, int flags);

/*
 * The prefix part of pattern must not contains wildcards.
 */
#define GFNM_PATHNAME 1		/* similar to FNM_PATHNAME */
#define GFNM_ONESTAR  2		/* there is only _one_ wildcard, a star */

extern int git_fnmatch(const char *pattern, const char *string,
		       int flags, int prefix);

#endif
