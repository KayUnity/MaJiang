#ifndef __DS_TREE_H
#define __DS_TREE_H

#include "DS_List.h"
#include "DS_Queue.h"

/// The namespace DataStructures was only added to avoid compiler errors for commonly named data structures
/// As these data structures are stand-alone, you can use them outside of HexNet for your own projects if you wish.
namespace DataStructures
{
	template <class TreeType>
	class Tree
	{
	public:
		Tree();
		Tree(TreeType &inputData);
		~Tree();
		void LevelOrderTraversal(DataStructures::List<Tree*> &output);
		void AddChild(TreeType &newData);
		void DeleteDecendants(void);

		TreeType data;
		DataStructures::List<Tree *> children;
	};

	template <class TreeType>
	Tree<TreeType>::Tree()
	{

	}

	template <class TreeType>
	Tree<TreeType>::Tree(TreeType &inputData)
	{
		data=inputData;
	}

	template <class TreeType>
	Tree<TreeType>::~Tree()
	{
	}

	template <class TreeType>
	void Tree<TreeType>::LevelOrderTraversal(DataStructures::List<Tree*> &output)
	{
		unsigned i;
		Tree<TreeType> *node;
		DataStructures::Queue<Tree<TreeType>*> queue;

		for (i=0; i < children.Size(); i++)
			queue.Push(children[i]);

		while (queue.Size())
		{
			node=queue.Pop();
			output.Insert(node);
			for (i=0; i < node->children.Size(); i++)
				queue.Push(node->children[i]);
		}
	}

	template <class TreeType>
	void Tree<TreeType>::AddChild(TreeType &newData)
	{
		children.Insert(new Tree(newData));
	}

	template <class TreeType>
	void Tree<TreeType>::DeleteDecendants(void)
	{
        DataStructures::List<Tree*> output;
		LevelOrderTraversal(output);
		unsigned i;
		for (i=0; i < output.Size(); i++)
			delete output[i];
	}
}

#endif
