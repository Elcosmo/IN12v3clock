#ifndef _HC595_H_
#define	_HC595_H_

#ifdef	__cplusplus
extern "C" {
#endif

void hc595Init (void);
void hc595OutputDisable (void);
void hc595OutputEnable (void);
//void hc595ChainShiftOut (uint16_t *data, uint8_t lenght);
void hc595ChainShiftOut (uint8_t *data, uint8_t lenght);
#ifdef	__cplusplus
}
#endif

#endif	/* _HC595_H_ */
