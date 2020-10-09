<?php

/**
 * Uses a binary tree stored in an array to implement a heap.
 */
final class TimerQueue
{
    /** @var TimerQueueEntry[] */
    private array $data = [];

    /** @var int[] */
    private array $pointers = [];

    public function insert(string $id, callable $callback, int $expiration): void
    {
        assert(!isset($this->pointers[$id]));

        $entry = new TimerQueueEntry($id, $callback, $expiration);

        $node = count($this->data);
        $this->data[$node] = $entry;
        $this->pointers[$id] = $node;

        while ($node !== 0 && $entry->expiration < $this->data[$parent = ($node - 1) >> 1]->expiration) {
            $temp = $this->data[$parent];
            $this->data[$node] = $temp;
            $this->pointers[$temp->id] = $node;

            $this->data[$parent] = $entry;
            $this->pointers[$id] = $parent;

            $node = $parent;
        }
    }

    public function remove(string $id): void
    {
        if (!isset($this->pointers[$id])) {
            return;
        }

        $this->removeAndRebuild($this->pointers[$id]);
    }

    public function extract(int $now): ?callable
    {
        if (empty($this->data)) {
            return null;
        }

        $data = $this->data[0];

        if ($data->expiration > $now) {
            return null;
        }

        $this->removeAndRebuild(0);

        return $data->callback;
    }

    /**
     * Returns the expiration time value at the top of the heap. Time complexity: O(1).
     *
     * @return int|null Expiration time of the watcher at the top of the heap or null if the heap is empty.
     */
    public function peek(): ?int
    {
        return isset($this->data[0]) ? $this->data[0]->expiration : null;
    }

    /**
     * @param int $node Remove the given node and then rebuild the data array from that node downward.
     *
     * @return void
     */
    private function removeAndRebuild(int $node): void
    {
        $length = count($this->data) - 1;
        $id = $this->data[$node]->id;
        $left = $this->data[$node] = $this->data[$length];
        $this->pointers[$left->id] = $node;
        unset($this->data[$length], $this->pointers[$id]);

        while (($child = ($node << 1) + 1) < $length) {
            if ($this->data[$child]->expiration < $this->data[$node]->expiration
                && ($child + 1 >= $length || $this->data[$child]->expiration < $this->data[$child + 1]->expiration)
            ) {
                // Left child is less than parent and right child.
                $swap = $child;
            } elseif ($child + 1 < $length && $this->data[$child + 1]->expiration < $this->data[$node]->expiration) {
                // Right child is less than parent and left child.
                $swap = $child + 1;
            } else { // Left and right child are greater than parent.
                break;
            }

            $left = $this->data[$node];
            $right = $this->data[$swap];

            $this->data[$node] = $right;
            $this->pointers[$right->id] = $node;

            $this->data[$swap] = $left;
            $this->pointers[$left->id] = $swap;

            $node = $swap;
        }
    }
}
