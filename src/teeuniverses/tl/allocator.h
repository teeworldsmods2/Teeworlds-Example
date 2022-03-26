/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef TEEUNIVERSES_TL_ALLOCATOR_H
#define TEEUNIVERSES_TL_ALLOCATOR_H

template <class T>
class tu_allocator_default
{
public:
	static T *alloc() { return new T; }
	static void free(T *p) { delete p; }

	static T *alloc_array(int size) { return new T [size]; }
	static void free_array(T *p) { delete [] p; }
	
	static void copy(T& a, const T& b) { a = b; }
	static void transfert(T& a, T& b) { a = b; }
};

template <class T>
class tu_allocator_copy
{
public:
	static T *alloc() { return new T; }
	static void free(T *p) { delete p; }

	static T *alloc_array(int size) { return new T [size]; }
	static void free_array(T *p) { delete [] p; }
	
	static void copy(T& a, const T& b) { a.copy(b); }
	static void transfert(T& a, T& b) { a.transfert(b); }
};

#endif // TL_FILE_ALLOCATOR_HPP
