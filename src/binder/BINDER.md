Okay. I won't give you a pretty intro or anything. I just need to figure out
this binder thing.

So, let's set up a design doc as to what the binder should do:
- Make sure any reference exists in said reference's scope(check that we can access what
  we're accessing)
- Make sure all files in a project can see all types.
- Make sure each file has to include packages on its own(if we do `package sys; use sys;` in `a.rct`, that shouldn't
  affect `b.rct`)
- Basic type checking(this should be relatively easy. Hopefully haha.)

Generics are a mess, so we'll just mark them as not done for now.