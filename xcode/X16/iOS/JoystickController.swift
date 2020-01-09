//
//  JoystickController.swift
//  CommanderX16
//
//  ; (C)2020 Matthew Pearce, License: 2-clause BSD
//

import Foundation
import JoyStickView

@objc
public protocol JoystickControllerDelegate {
	func joystickPositionUpdated(angle: CGFloat, displacement: CGFloat)
}

@objc
public class JoystickController: NSObject {

	@objc
	public var joyStick: JoyStickView?
	var delegate: JoystickControllerDelegate?

    let joystickOffset: CGFloat = 60.0
    let joystickSpan: CGFloat = 80.0

	@objc
	public func getJoyStickFor(delegate: JoystickControllerDelegate) -> JoyStickView {

		self.delegate = delegate
		let stick = makeJoystick()
		let monitor: JoyStickViewPolarMonitor = { report in
			if report.displacement > 0.0 {
				self.delegate?.joystickPositionUpdated(angle: report.angle, displacement: report.displacement)
			}
		}
		stick.monitor = .polar(monitor: monitor)
		joyStick = stick
		return stick
	}

	private func makeJoystick() -> JoyStickView {
        let frame = CGRect(origin: CGPoint(x: 0.0, y: 0.0), size: CGSize(width: joystickSpan, height: joystickSpan))
        let joystick = JoyStickView(frame: frame)
		joystick.backgroundColor = .clear
		joystick.baseImage = UIImage(named: "FancyBase")
		joystick.handleImage = UIImage(named: "FancyHandle")
		joystick.movable = false;
        joystick.alpha = 1
        joystick.baseAlpha = 0.75
        joystick.colorFillHandleImage = true
		joystick.travel = 0.5
        return joystick
    }
}

