#ifndef __SAFEGL_H
#define __SAFEGL_H

void safeGlNewList( GLuint list, GLenum mode );
void safeGlEndList();

#define glNewList	safeGlNewList
#define glEndList	safeGlEndList

#endif
