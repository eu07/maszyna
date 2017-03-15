| nazwa                  | typ   | opis                                                       |
|------------------------|-------|------------------------------------------------------------|
| direction              | int   | kiernek jazdy (-1: do ty³u, 0: ¿aden, 1: do przodu)        |
| slipping_wheels        | bool  | poœlizg                                                    |
| converter              | bool  | dzia³anie przetwornicy                                     |
| main_ctrl_actual_pos   | int   | numer pozycji nastawnika (wskaŸnik RList)                  |
| scnd_ctrl_actual_pos   | int   | numer pozycji bocznika (wskaŸnik MotorParam)               |
| fuse                   | bool  | wy³¹cznik szybki (bezpiecznik nadmiarowy)                  |
| converter_overload     | bool  | wy³¹cznik nadmiarowy przetwornicy i ogrzewania             |
| voltage                | float | napiêcie w sieci                                           |
| velocity               | float | aktualna prêdkoœæ                                          |
| im                     | float | pr¹d silnika                                               |
| compress               | bool  | dzia³anie sprê¿arki                                        |
| hours                  | int   | aktualny czas                                              |
| minutes                | int   | aktualny czas                                              |
| seconds                | int   | aktualny czas                                              |
| velocity_desired       | float | prêdkosæ zadana                                            |
