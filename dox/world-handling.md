
## world handling

1.  the thread that exchanged old world for new world is responsible for the old world, except if it was locked
2.  the thread that locked old world is responsible for it, when it unlocks the world successfully, it is no longer responsible 
3.  a world that is locked, can only be unlocked by the same thread. if it is implicitly unlocked by a free or a checkin, then a new world must replace the locked world. Otherwise ownership of the previously locked world would be unclear, since the world could be freed now normally (1)
4.  worlds need to be delay released because of ABA

## the register problem

* if the thread fails to register, the world can not be delay released yet, it must be saved until eventually
* the thread registers, then all locked worlds can be delay released as above

## the unregister problem

* the old world can not be delay released, because the thread is not active anymore. it has to be put on the leak list
* the leak list can only be freed if no thread will register again, outside of library control





