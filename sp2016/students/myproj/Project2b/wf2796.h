#if !(_wf2796_h)
#define _wf2796_h 1

#include "LifeForm.h"
#include "Init.h"

class wf2796 : public LifeForm {
protected:
  bool course_changed;
  static void initialize(void);
  void spawn(void);
  void hunt(void);
  void live(void);
  Event* hunt_event;
public:
  wf2796(void);
  ~wf2796(void);
  Color my_color(void) const;   // defines LifeForm::my_color
  static LifeForm* create(void);
  std::string species_name(void) const;
  std::string player_name(void) const;
  Action encounter(const ObjInfo&);
  friend class Initializer<wf2796>;
};


#endif /* !(_wf2796_h) */
