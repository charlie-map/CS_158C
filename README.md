# CS_158 in C
### Charlie Hall

Working through implementations of many of the projects in CS 158 at Brown University. The following are the main components in the projects:

- [Parse](#Tokenize-Document)
- [Index](#Serialize-Token)
- [Deserialize](#Deserialize-Index)
- [Query](#Query-Index)
- [Classify](#Classify-Document)

# Tokenize Document
This searches through XML (Extended Markup Language) to recursively generate a tree (similar to the Document Object Model). The main functions within the tokenize process (which is laid in in `token.h`) are:
- `token_t *tokenize(char filetype, char *filename);`
  - Most of the tree creation occurs within this function here. This reads technically character by character. However, when important tags within the markup language are seen (such as an open carrot `<`), reads the tag from inside the other close carrot (`>`) and then _recursively_ (this uses a stack under the hood, but a similar concept) repeats the process on sub tags of that branch. The filetype can either be `f` for a filename or `s` for a `char *`
  - This takes a `char *` which holds the file name of the XML page (or the XML page itself), which could look like:
  ```XML
  <page>
    <id>10</id>
    <title>Hello there</title>
    <text>Nice XML page!</text>
  </page>
  ```
  which can then easily be read into a `token_t` variable by calling tokenize with the name of the file, say "miniXML.txt":
  ```C
  token_t *new_tree = tokenize('f', "miniXML.txt");
  ```
  or with the `char *` XML page:
  ```C
  token_t *new_tree = tokenize('s', "<page>\n<inside>\nTags\n</inside>\n</page>");
  ```
  - The returned `new_tree` value points to a `NULL` data root node (aptly labelled `root` for the tag) where everything is a child of said root tag. This means that the XML file that gets read in can have multiple tags at the root level like the following:
  ```XML
  <page>A page about crocodiles</page>
  <page>A page about kangaroos</page>
  <page>A page about zebras</page>
  ```
- `token_t **token_children(token_t *parent);` [note 1](#Notes)
  - Grabs all children of a `token_t`. In the previoous XML example. The children array would contain the tags: `["id", "title", "text"]`.
- `token_t *grab_token_parent(token_t *curr_token);` [note 1](#Notes)
  - Returns the parent of the current `token_t`.
- `token_t *grab_token_by_tag(token_t *search_token, char *tag_name);`
  - BFS for the first occurence of a tag that is equivalent to `tag_name`. Returns `NULL` if no tag is found that matches.
- `token_t **grab_tokens_by_tag(token_t *start_token, char *tags_name, int *spec_token_max);`
  - DFS for all occurences of `tags_name`. `spec_token_max` is passed in as an `int *` and after the function call will hold the length of the `token_t **`. Returns an empty (no values) `token_t **` if no tag is found.
- `char *token_read_all_data(token_t *search_token, int *data_max);`
  - DFS through search_token and children and allocates a `char *` that contains all of the data concatenated. `data_max` will contain the length of the `char *`. Returns an empty string if there is no data.
- `char **token_get_tag_data(token_t *search_token, char *tag_name, int *max_tag);`
  - Given a start node in the `token_t` tree, searches for `tag_name` and returns an array of that data for all the matches of said `tag_name`. `int *max_tag` must be preallocated and will have the length of the returned list.
  - Using the `new_tree` defined in the previous example, the following example shows getting all the instances of `id` from the tree starting at the root position:
  ```C
  int *max_length = malloc(sizeof(int));
  *max_length = 0;
  char **returned_tags = token_get_tag_data(new_tree, "id", max_length);
  ```
  - At which point, each value in `returned_tags` can be evaluated using a loop from `0` to `*max_length`.
  - *Note:* Both `max_length` and `returned_tags` require a single free such as the following:
  ```C
  free(max_length);
  free(returned_tags);
  ```
- `int destroy_token(token_t *curr_token);`
  - Recursively frees the entire tree previously created

# Serialize Token
The serialize process will write out to files in some form. Currently, there is one function type that can be used:
- Word bag:
```C
int word_bag(FILE *index_fp, FILE *title_fp, trie_t *stopword_trie, token_t *full_page);
```

This will write to an index file (`index_fp`) in the format:
```
id magnitude word:frequency word:frequency word:frequency etc.
```
and for the title index (`title_fp`) in the format:
```
id: title
```
- Other ideas for serialization? Add an issue to this repository.

# Deserialize Index
Deserialize quickly pulls all the documents from the serialized form and places them each in a separate hashmap. This is possible because instead of creating a new `char *` for each word, the same `char *` used for `idf` is used for each occurence of the word in any document, which dramatically improves space complexity.

# Query Index
#### _Coming Soon_

# Classify Document
Classify currently creates `K` (based on input) centroids and each document will try to attach to a singular centroid. The centroids continue trying to "move" towards the center of their clusters (mean-finding) until little enough has changed that the program can stop. Currently with a run of the program with `K=4` and the first four documents are chosen as the initial cluster centroids (not actually chosen, but copied), the following results occur:

- Group 1:
  - Ball
  - Main Page
  - Langton's ant
  - Chicago Fire
  - Lacrosse
  - Basketball
  - Virginia Cavaliers men's basketball
  - Virginia Cavaliers
- Group 2:
  - Spain
  - Abortion debate
  - Abortion in the United States by state
  - Minors and abortion
  - Potato
  - Alan Turing
  - Ancient Rome
  - Great Chicago Fire
  - Marsden Rock
  - Wiki
  - Final Fantasy XIII-2
  - Apple Inc.
- Group 3:
  - Europe
  - Nicholas II of Russia
  - Ancient Greece
  - Tree Frog
- Group 4:
  - Greater Caucasus
 
 Noticably, there is a large weighting towards Group 2 for any "random" documents. This is likely dude to the choice of Spain as the start document; that document contains many thousands of words, and therefore will have some higher weighting since the reach of the words spreads very far, from `Final Fantasy XIII-2` to `Marsden Rock`.
 
 ### Future Updates and Enhancements:
 The addition of multiple K-means with different start centroids occuring at the same time concurrently (through the use of `pthread`s). This will then allow a weighting to occur on each of the returned K-mean groups to then choose the "best" one.

# Notes
  1. _This function currently is not used outside the scope of the internal project. Add comments for possible additions_
